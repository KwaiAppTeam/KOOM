/*
 * Copyright (C) 2012 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define LOG_TAG "memory_map"
#include "memory_map.h"

#include <ctype.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <inttypes.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>

#include "android-base/stringprintf.h"

#if defined(__LP64__)
#define PAD_PTR "016" PRIxPTR
#else
#define PAD_PTR "08" PRIxPTR
#endif

#ifndef NT_GNU_BUILD_ID
#define NT_GNU_BUILD_ID 3
#endif

#ifndef ElfW
#define ElfW(type) Elf_##type
#endif

struct BuildIdNote {
  ElfW(Nhdr) nhdr;
  char name[4]; /* Note name for build-id is "GNU\0" */
  uint8_t build_id[0];
};

struct CallbackData {
  void *dli_fbase;
  FrameInfo frame_info;

  CallbackData(void *fbase) : dli_fbase(fbase) {}
};

#if __ANDROID_API__ < __ANDROID_API_L__
extern "C" __attribute__((weak)) int dl_iterate_phdr(
    int (*)(struct dl_phdr_info *, size_t, void *), void *);
#endif

// Format of /proc/<PID>/maps:
// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /system/lib/libcomposer.so
static MapEntry *ParseLine(char *line) {
  uintptr_t start;
  uintptr_t end;
  uintptr_t offset;
  int flags;
  char permissions[5];
  int name_pos;
  if (sscanf(line, "%" PRIxPTR "-%" PRIxPTR " %4s %" PRIxPTR " %*x:%*x %*d %n",
             &start, &end, permissions, &offset, &name_pos) < 2) {
    return nullptr;
  }

  const char *name = line + name_pos;
  size_t name_len = strlen(name);
  if (name_len && name[name_len - 1] == '\n') {
    name_len -= 1;
  }

  flags = 0;
  if (permissions[0] == 'r') {
    flags |= PROT_READ;
  }
  if (permissions[2] == 'x') {
    flags |= PROT_EXEC;
  }

  MapEntry *entry = new MapEntry(start, end, offset, name, name_len, flags);
  if (!(flags & PROT_READ)) {
    // Any unreadable map will just get a zero load bias.
    entry->load_bias = 0;
    entry->init = true;
    entry->valid = false;
  }
  return entry;
}

// Read process memory through /proc/self/mem instead of dereferencing
// directly. A map entry may become stale (dlclose/munmap) between reading
// /proc/self/maps and this lazy validation, so a raw dereference can crash
// with SIGSEGV. pread fails gracefully on unmapped pages.
static bool SafeReadMap(uintptr_t addr, void *buf, size_t len) {
  static int mem_fd = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
  if (mem_fd < 0) {
    return false;
  }
  return pread(mem_fd, buf, len, static_cast<off64_t>(addr)) ==
         static_cast<ssize_t>(len);
}

template <typename T>
static inline bool GetVal(MapEntry *entry, uintptr_t addr, T *store) {
  if (!(entry->flags & PROT_READ) || addr < entry->start ||
      addr + sizeof(T) > entry->end) {
    return false;
  }
  // Make sure the address is aligned properly.
  if (addr & (sizeof(T) - 1)) {
    return false;
  }
  return SafeReadMap(addr, store, sizeof(T));
}

static bool ValidElf(MapEntry *entry) {
  uintptr_t addr = entry->start;
  uintptr_t end;
  if (__builtin_add_overflow(addr, SELFMAG, &end) || end >= entry->end) {
    return false;
  }

  char mag[SELFMAG];
  if (!SafeReadMap(addr, mag, SELFMAG)) {
    return false;
  }
  return memcmp(mag, ELFMAG, SELFMAG) == 0;
}

static void ReadLoadbias(MapEntry *entry) {
  entry->load_bias = 0;
  uintptr_t addr = entry->start;
  ElfW(Ehdr) ehdr;
  if (!GetVal<ElfW(Half)>(entry, addr + offsetof(ElfW(Ehdr), e_phnum),
                          &ehdr.e_phnum)) {
    return;
  }
  if (!GetVal<ElfW(Off)>(entry, addr + offsetof(ElfW(Ehdr), e_phoff),
                         &ehdr.e_phoff)) {
    return;
  }
  addr += ehdr.e_phoff;
  for (size_t i = 0; i < ehdr.e_phnum; i++) {
    ElfW(Phdr) phdr;
    if (!GetVal<ElfW(Word)>(entry, addr + offsetof(ElfW(Phdr), p_type),
                            &phdr.p_type)) {
      return;
    }
    if (!GetVal<ElfW(Word)>(entry, addr + offsetof(ElfW(Phdr), p_flags),
                            &phdr.p_flags)) {
      return;
    }
    if (!GetVal<ElfW(Off)>(entry, addr + offsetof(ElfW(Phdr), p_offset),
                           &phdr.p_offset)) {
      return;
    }
    if ((phdr.p_type == PT_LOAD) && (phdr.p_flags & PF_X)) {
      if (!GetVal<ElfW(Addr)>(entry, addr + offsetof(ElfW(Phdr), p_vaddr),
                              &phdr.p_vaddr)) {
        return;
      }
      if (phdr.p_vaddr >= phdr.p_offset) {
        entry->load_bias = phdr.p_vaddr - phdr.p_offset;
      } else {
        entry->load_bias = 0;
      }
      return;
    }
    addr += sizeof(phdr);
  }
}

static void inline Init(MapEntry *entry) {
  if (entry->init) {
    return;
  }
  entry->init = true;
  if (ValidElf(entry)) {
    entry->valid = true;
    ReadLoadbias(entry);
  }
}

static int FrameInfoCallback(struct dl_phdr_info *info, size_t size,
                             void *data_) {
  if (!info || !info->dlpi_name) return 0;

  CallbackData *callback_data = reinterpret_cast<CallbackData *>(data_);

  /* Calculate address where shared object is mapped into the process space.
   * (Using the base address and the virtual address of the first LOAD segment)
   */
  void *map_start = nullptr;
  for (unsigned i = 0; i < info->dlpi_phnum; i++) {
    if (info->dlpi_phdr[i].p_type == PT_LOAD) {
      map_start = (void *)(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
      break;
    }
  }

  if (map_start != callback_data->dli_fbase) return 0;

  callback_data->frame_info.load_base = info->dlpi_addr;
  callback_data->frame_info.name = info->dlpi_name;
  for (unsigned i = 0; i < info->dlpi_phnum; i++) {
    if (info->dlpi_phdr[i].p_type != PT_NOTE) continue;

    BuildIdNote *note = reinterpret_cast<BuildIdNote *>(
        (info->dlpi_addr + info->dlpi_phdr[i].p_vaddr));
    ptrdiff_t len = info->dlpi_phdr[i].p_filesz;

    while (len >= sizeof(BuildIdNote)) {
      if (note->nhdr.n_type == NT_GNU_BUILD_ID && note->nhdr.n_descsz != 0 &&
          note->nhdr.n_namesz == 4 && memcmp(note->name, "GNU", 4) == 0) {
        std::string raw_build_id;
        raw_build_id.append(reinterpret_cast<char *>(note->build_id),
                            note->nhdr.n_descsz);

        for (const char &c : raw_build_id) {
          // Use %hhx to avoid sign extension on abis that have signed chars.
          callback_data->frame_info.build_id +=
              android::base::StringPrintf("%02hhx", c);
        }
        return 1;
      }

#define ALIGN_POT(x, pot_align) (((x) + (pot_align)-1) & ~((pot_align)-1))

      size_t offset = sizeof(ElfW(Nhdr)) + ALIGN_POT(note->nhdr.n_namesz, 4) +
                      ALIGN_POT(note->nhdr.n_descsz, 4);
      note = (BuildIdNote *)((char *)note + offset);
      len -= offset;
    }
  }

  return 0;
}

bool MemoryMap::ReadMaps() {
  FILE *fp = fopen("/proc/self/maps", "re");
  if (fp == nullptr) {
    return false;
  }

  std::vector<char> buffer(1024);
  while (fgets(buffer.data(), buffer.size(), fp) != nullptr) {
    MapEntry *entry = ParseLine(buffer.data());
    if (entry == nullptr) {
      fclose(fp);
      return false;
    }

    auto it = entries_.find(entry);
    if (it == entries_.end()) {
      entries_.insert(entry);
    } else {
      delete entry;
    }
  }
  fclose(fp);
  return true;
}

MemoryMap::~MemoryMap() {
  for (auto *entry : entries_) {
    delete entry;
  }
  entries_.clear();
}

bool MemoryMap::FillFrameInfo(uintptr_t pc, FrameInfo &frame_info) {
  Dl_info dl_info;

  if (!dladdr(reinterpret_cast<void *>(pc), &dl_info)) {
    return false;
  }

  if (!dl_info.dli_fbase) {
    return false;
  }

  CallbackData callback_data(dl_info.dli_fbase);

  if (!dl_iterate_phdr(FrameInfoCallback, &callback_data)) {
    return false;
  }

  if (pc <= callback_data.frame_info.load_base) {
    return false;
  }
  callback_data.frame_info.rel_pc = pc - callback_data.frame_info.load_base;
  frame_info = callback_data.frame_info;

  if (frame_info.IsSystemFrame()) {
    const char *symbol = dl_info.dli_sname;
    if (symbol != nullptr) {
      // 记录符号起始地址，供上层格式化 "symbol+offset"（与原 FormatSymbol 输出对齐）
      frame_info.sym_addr = reinterpret_cast<uintptr_t>(dl_info.dli_saddr);
      char *demangled_name =
          abi::__cxa_demangle(symbol, nullptr, nullptr, nullptr);
      const char *name;
      if (demangled_name != nullptr) {
        name = demangled_name;
      } else {
        name = symbol;
      }
      frame_info.symbol = name;
      free(demangled_name);
    }
  }
  return true;
}

std::string MemoryMap::FormatFrameSymbol(const FrameInfo &frame_info,
                                          uintptr_t pc) {
  char buf[1024];
  if (!frame_info.symbol.empty() && frame_info.sym_addr != 0 &&
      pc >= frame_info.sym_addr) {
    snprintf(buf, sizeof(buf), "  %s (%s+%" PRIuPTR ")\n",
             frame_info.name.c_str(), frame_info.symbol.c_str(),
             pc - frame_info.sym_addr);
  } else {
    snprintf(buf, sizeof(buf), "  %s\n", frame_info.name.c_str());
  }
  return buf;
}

MapEntry *MemoryMap::CalculateRelPc(uintptr_t pc, uintptr_t *rel_pc) {
  MapEntry pc_entry(pc);

  auto it = entries_.find(&pc_entry);
  if (it == entries_.end()) {
    ReadMaps();
  }
  it = entries_.find(&pc_entry);
  if (it == entries_.end()) {
    return nullptr;
  }

  MapEntry *entry = *it;
  Init(entry);

  if (rel_pc != nullptr) {
    // Need to check to see if this is a read-execute map and the read-only
    // map is the previous one.
    if (!entry->valid && it != entries_.begin()) {
      MapEntry *prev_entry = *--it;
      if (prev_entry->flags == PROT_READ &&
          prev_entry->offset < entry->offset &&
          prev_entry->name == entry->name) {
        Init(prev_entry);

        if (prev_entry->valid) {
          entry->elf_start_offset = prev_entry->offset;
          *rel_pc = pc - entry->start + (entry->offset - prev_entry->offset) + prev_entry->load_bias;
          return entry;
        }
      }
    }
    *rel_pc = pc - entry->start + entry->load_bias;
  }
  return entry;
}

std::string MemoryMap::FormatSymbol(MapEntry *entry, uintptr_t pc) {
  std::string str;
  uintptr_t offset = 0;
  const char *symbol = nullptr;

  Dl_info info;
  if (dladdr(reinterpret_cast<void *>(pc), &info) != 0) {
    offset = reinterpret_cast<uintptr_t>(info.dli_saddr);
    symbol = info.dli_sname;
  } else {
    info.dli_fname = nullptr;
  }

  const char *soname =
      (entry != nullptr) ? entry->name.c_str() : info.dli_fname;
  if (soname == nullptr) {
    soname = "<unknown>";
  }

  char offset_buf[128];
  if (entry != nullptr && entry->elf_start_offset != 0) {
    snprintf(offset_buf, sizeof(offset_buf), " (offset 0x%" PRIxPTR ")",
             entry->elf_start_offset);
  } else {
    offset_buf[0] = '\0';
  }

  char buf[1024];
  if (symbol != nullptr) {
    char *demangled_name =
        abi::__cxa_demangle(symbol, nullptr, nullptr, nullptr);
    const char *name;
    if (demangled_name != nullptr) {
      name = demangled_name;
    } else {
      name = symbol;
    }
    snprintf(buf, sizeof(buf), "  %s%s (%s+%" PRIuPTR ")\n", soname, offset_buf,
             name, pc - offset);
    free(demangled_name);
  } else {
    snprintf(buf, sizeof(buf), "  %s%s\n", soname, offset_buf);
  }
  str += buf;

  return std::move(str);
}
