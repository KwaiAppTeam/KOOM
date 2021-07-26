/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "art_api/dex_file_external.h"

#include <inttypes.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/mapped_file.h>
#include <android-base/stringprintf.h>

#include <dex/class_accessor-inl.h>
#include <dex/code_item_accessors-inl.h>
#include <dex/dex_file-inl.h>
#include <dex/dex_file_loader.h>

namespace art {
namespace {

struct MethodCacheEntry {
  int32_t offset; // Offset relative to the start of the dex file header.
  int32_t len;
  int32_t index; // Method index.
};

class MappedFileContainer : public DexFileContainer {
public:
  explicit MappedFileContainer(std::unique_ptr<android::base::MappedFile> &&map)
      : map_(std::move(map)) {}
  ~MappedFileContainer() override {}
  int GetPermissions() override { return 0; }
  bool IsReadOnly() override { return true; }
  bool EnableWrite() override { return false; }
  bool DisableWrite() override { return false; }

private:
  std::unique_ptr<android::base::MappedFile> map_;
  DISALLOW_COPY_AND_ASSIGN(MappedFileContainer);
};

} // namespace
} // namespace art

extern "C" {

struct ExtDexFileString {
  const std::string str_;
};

static const ExtDexFileString empty_string{""};

const ExtDexFileString *ExtDexFileMakeString(const char *str, size_t size) {
  if (size == 0) {
    return &empty_string;
  }
  return new ExtDexFileString{std::string(str, size)};
}

const char *ExtDexFileGetString(const ExtDexFileString *ext_string, /*out*/ size_t *size) {
  DCHECK(ext_string != nullptr);
  *size = ext_string->str_.size();
  return ext_string->str_.data();
}

void ExtDexFileFreeString(const ExtDexFileString *ext_string) {
  DCHECK(ext_string != nullptr);
  if (ext_string != &empty_string) {
    delete (ext_string);
  }
}

// Wraps DexFile to add the caching needed by the external interface. This is
// what gets passed over as ExtDexFile*.
struct ExtDexFile {
public:
  std::unique_ptr<const art::DexFile> dex_file_;
  explicit ExtDexFile(std::unique_ptr<const art::DexFile> &&dex_file)
      : dex_file_(std::move(dex_file)) {}

  art::MethodCacheEntry *GetMethodCacheEntryForOffset(int64_t dex_offset) {
    // First look in the method cache.
    auto it = method_cache_.upper_bound(dex_offset);
    if (it != method_cache_.end() && dex_offset >= it->second.offset) {
      return &it->second;
    }

    uint32_t class_def_index;
    if (GetClassDefIndex(dex_offset, &class_def_index)) {
      art::ClassAccessor accessor(*dex_file_, class_def_index);

      for (const art::ClassAccessor::Method &method : accessor.GetMethods()) {
        art::CodeItemInstructionAccessor code = method.GetInstructions();
        if (!code.HasCodeItem()) {
          continue;
        }

        int32_t offset = reinterpret_cast<const uint8_t *>(code.Insns()) - dex_file_->Begin();
        int32_t len = code.InsnsSizeInBytes();
        if (offset <= dex_offset && dex_offset < offset + len) {
          int32_t index = method.GetIndex();
          auto res = method_cache_.emplace(offset + len, art::MethodCacheEntry{offset, len, index});
          return &res.first->second;
        }
      }
    }

    return nullptr;
  }

private:
  bool GetClassDefIndex(uint32_t dex_offset, uint32_t *class_def_index) {
    if (class_cache_.empty()) {
      // Create binary search table with (end_dex_offset, class_def_index) entries.
      // That is, we don't assume that dex code of given class is consecutive.
      std::deque<std::pair<uint32_t, uint32_t>> cache;
      for (art::ClassAccessor accessor : dex_file_->GetClasses()) {
        for (const art::ClassAccessor::Method &method : accessor.GetMethods()) {
          art::CodeItemInstructionAccessor code = method.GetInstructions();
          if (code.HasCodeItem()) {
            int32_t offset = reinterpret_cast<const uint8_t *>(code.Insns()) - dex_file_->Begin();
            DCHECK_NE(offset, 0);
            cache.emplace_back(offset + code.InsnsSizeInBytes(), accessor.GetClassDefIndex());
          }
        }
      }
      std::sort(cache.begin(), cache.end());

      // If two consecutive methods belong to same class, we can merge them.
      // This tends to reduce the number of entries (used memory) by 10x.
      size_t num_entries = cache.size();
      if (cache.size() > 1) {
        for (auto it = std::next(cache.begin()); it != cache.end(); it++) {
          if (std::prev(it)->second == it->second) {
            std::prev(it)->first = 0; // Clear entry with lower end_dex_offset (mark to remove).
            num_entries--;
          }
        }
      }

      // The cache is immutable now. Store it as continuous vector to save space.
      class_cache_.reserve(num_entries);
      auto pred = [](auto it) { return it.first != 0; }; // Entries to copy (not cleared above).
      std::copy_if(cache.begin(), cache.end(), std::back_inserter(class_cache_), pred);
    }

    // Binary search in the class cache. First element of the pair is the key.
    auto comp = [](uint32_t value, const auto &it) { return value < it.first; };
    auto it = std::upper_bound(class_cache_.begin(), class_cache_.end(), dex_offset, comp);
    if (it != class_cache_.end()) {
      *class_def_index = it->second;
      return true;
    }
    return false;
  }

  // Binary search table with (end_dex_offset, class_def_index) entries.
  std::vector<std::pair<uint32_t, uint32_t>> class_cache_;
  std::map<uint32_t, art::MethodCacheEntry> method_cache_; // end_dex_offset -> method.
};

int ExtDexFileOpenFromMemory(const void *addr,
                             /*inout*/ size_t *size, const char *location,
                             /*out*/ const ExtDexFileString **ext_error_msg,
                             /*out*/ ExtDexFile **ext_dex_file) {
  if (*size < sizeof(art::DexFile::Header)) {
    *size = sizeof(art::DexFile::Header);
    *ext_error_msg = nullptr;
    return false;
  }

  const art::DexFile::Header *header = reinterpret_cast<const art::DexFile::Header *>(addr);
  uint32_t file_size = header->file_size_;
  if (art::CompactDexFile::IsMagicValid(header->magic_)) {
    // Compact dex files store the data section separately so that it can be shared.
    // Therefore we need to extend the read memory range to include it.
    // TODO: This might be wasteful as we might read data in between as well.
    //       In practice, this should be fine, as such sharing only happens on disk.
    uint32_t computed_file_size;
    if (__builtin_add_overflow(header->data_off_, header->data_size_, &computed_file_size)) {
      *ext_error_msg = new ExtDexFileString{
          android::base::StringPrintf("Corrupt CompactDexFile header in '%s'", location)};
      return false;
    }
    if (computed_file_size > file_size) {
      file_size = computed_file_size;
    }
  } else if (!art::StandardDexFile::IsMagicValid(header->magic_)) {
    *ext_error_msg = new ExtDexFileString{
        android::base::StringPrintf("Unrecognized dex file header in '%s'", location)};
    return false;
  }

  if (*size < file_size) {
    *size = file_size;
    *ext_error_msg = nullptr;
    return false;
  }

  std::string loc_str(location);
  art::DexFileLoader loader;
  std::string error_msg;
  std::unique_ptr<const art::DexFile> dex_file =
      loader.Open(static_cast<const uint8_t *>(addr), *size, loc_str, header->checksum_,
                  /*oat_dex_file=*/nullptr,
                  /*verify=*/false,
                  /*verify_checksum=*/false, &error_msg);
  if (dex_file == nullptr) {
    *ext_error_msg = new ExtDexFileString{std::move(error_msg)};
    return false;
  }

  *ext_dex_file = new ExtDexFile(std::move(dex_file));
  return true;
}

int ExtDexFileOpenFromFd(int fd, off_t offset, const char *location,
                         /*out*/ const ExtDexFileString **ext_error_msg,
                         /*out*/ ExtDexFile **ext_dex_file) {
  size_t length;
  {
    struct stat sbuf;
    std::memset(&sbuf, 0, sizeof(sbuf));
    if (fstat(fd, &sbuf) == -1) {
      *ext_error_msg = new ExtDexFileString{
          android::base::StringPrintf("fstat '%s' failed: %s", location, std::strerror(errno))};
      return false;
    }
    if (S_ISDIR(sbuf.st_mode)) {
      *ext_error_msg = new ExtDexFileString{
          android::base::StringPrintf("Attempt to mmap directory '%s'", location)};
      return false;
    }
    length = sbuf.st_size;
  }

  if (length < offset + sizeof(art::DexFile::Header)) {
    *ext_error_msg = new ExtDexFileString{android::base::StringPrintf(
        "Offset %" PRId64 " too large for '%s' of size %zu", int64_t{offset}, location, length)};
    return false;
  }

  // Cannot use MemMap in libartbase here, because it pulls in dlopen which we
  // can't have when being compiled statically.
  std::unique_ptr<android::base::MappedFile> map =
      android::base::MappedFile::FromFd(fd, offset, length, PROT_READ);
  if (map == nullptr) {
    *ext_error_msg = new ExtDexFileString{
        android::base::StringPrintf("mmap '%s' failed: %s", location, std::strerror(errno))};
    return false;
  }

  const art::DexFile::Header *header = reinterpret_cast<const art::DexFile::Header *>(map->data());
  uint32_t file_size;
  if (__builtin_add_overflow(offset, header->file_size_, &file_size)) {
    *ext_error_msg =
        new ExtDexFileString{android::base::StringPrintf("Corrupt header in '%s'", location)};
    return false;
  }
  if (length < file_size) {
    *ext_error_msg = new ExtDexFileString{
        android::base::StringPrintf("Dex file '%s' too short: expected %" PRIu32 ", got %" PRIu64,
                                    location, file_size, uint64_t{length})};
    return false;
  }

  void *addr = map->data();
  size_t size = map->size();
  auto container = std::make_unique<art::MappedFileContainer>(std::move(map));

  std::string loc_str(location);
  std::string error_msg;
  art::DexFileLoader loader;
  std::unique_ptr<const art::DexFile> dex_file =
      loader.Open(reinterpret_cast<const uint8_t *>(addr), size, loc_str, header->checksum_,
                  /*oat_dex_file=*/nullptr,
                  /*verify=*/false,
                  /*verify_checksum=*/false, &error_msg, std::move(container));
  if (dex_file == nullptr) {
    *ext_error_msg = new ExtDexFileString{std::move(error_msg)};
    return false;
  }
  *ext_dex_file = new ExtDexFile(std::move(dex_file));
  return true;
}

int ExtDexFileGetMethodInfoForOffset(ExtDexFile *ext_dex_file, int64_t dex_offset,
                                     int with_signature,
                                     /*out*/ ExtDexFileMethodInfo *method_info) {
  if (!ext_dex_file->dex_file_->IsInDataSection(ext_dex_file->dex_file_->Begin() + dex_offset)) {
    return false; // The DEX offset is not within the bytecode of this dex file.
  }

  if (ext_dex_file->dex_file_->IsCompactDexFile()) {
    // The data section of compact dex files might be shared.
    // Check the subrange unique to this compact dex.
    const art::CompactDexFile::Header &cdex_header =
        ext_dex_file->dex_file_->AsCompactDexFile()->GetHeader();
    uint32_t begin = cdex_header.data_off_ + cdex_header.OwnedDataBegin();
    uint32_t end = cdex_header.data_off_ + cdex_header.OwnedDataEnd();
    if (dex_offset < begin || dex_offset >= end) {
      return false; // The DEX offset is not within the bytecode of this dex file.
    }
  }

  art::MethodCacheEntry *entry = ext_dex_file->GetMethodCacheEntryForOffset(dex_offset);
  if (entry != nullptr) {
    method_info->offset = entry->offset;
    method_info->len = entry->len;
    method_info->name =
        new ExtDexFileString{ext_dex_file->dex_file_->PrettyMethod(entry->index, with_signature)};
    return true;
  }

  return false;
}

void ExtDexFileGetAllMethodInfos(ExtDexFile *ext_dex_file, int with_signature,
                                 ExtDexFileMethodInfoCallback *method_info_cb, void *user_data) {
  for (art::ClassAccessor accessor : ext_dex_file->dex_file_->GetClasses()) {
    for (const art::ClassAccessor::Method &method : accessor.GetMethods()) {
      art::CodeItemInstructionAccessor code = method.GetInstructions();
      if (!code.HasCodeItem()) {
        continue;
      }

      ExtDexFileMethodInfo method_info;
      method_info.offset = static_cast<int32_t>(reinterpret_cast<const uint8_t *>(code.Insns()) -
                                                ext_dex_file->dex_file_->Begin());
      method_info.len = code.InsnsSizeInBytes();
      method_info.name = new ExtDexFileString{
          ext_dex_file->dex_file_->PrettyMethod(method.GetIndex(), with_signature)};
      method_info_cb(&method_info, user_data);
    }
  }
}

void ExtDexFileFree(ExtDexFile *ext_dex_file) { delete (ext_dex_file); }

} // extern "C"