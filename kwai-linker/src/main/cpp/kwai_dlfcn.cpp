// Copyright 2020 Kwai, Inc. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//         http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//         limitations under the License.

// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <kwai_dlfcn.h>
#include <kwai_log.h>
#include <kwai_macros.h>
#include <link.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

namespace kwai {
namespace linker {

extern "C" int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info *, size_t, void *),
                               void *__data) __attribute__((weak));
int dl_iterate_phdr_wrapper(int (*__callback)(struct dl_phdr_info *, size_t, void *),
                            void *__data) {
  if (dl_iterate_phdr) {
    return dl_iterate_phdr(__callback, __data);
  }
  ALOGF("dl_iterate_phdr unsupported!");
  return 0;
}

int DlFcn::android_api_;

void DlFcn::init_api() {
  android_api_ = android_get_device_api_level();
  ALOGD("android_api_ = %d", android_api_);
}

int DlFcn::dl_iterate_callback(dl_phdr_info *info, size_t size, void *data) {
  ALOGD("dl_iterate_callback %s %p", info->dlpi_name, info->dlpi_addr);
  auto target = reinterpret_cast<dl_iterate_data *>(data);
  if (info->dlpi_addr != 0 && strstr(info->dlpi_name, target->info_.dlpi_name)) {
    target->info_.dlpi_addr = info->dlpi_addr;
    target->info_.dlpi_phdr = info->dlpi_phdr;
    target->info_.dlpi_phnum = info->dlpi_phnum;
    target->info_.dlpi_name = info->dlpi_name;
    // break iterate
    return 1;
  }
  // continue iterate
  return 0;
}

using __loader_dlopen_fn = void *(*)(const char *filename, int flag, void *address);

KWAI_EXPORT void *DlFcn::dlopen(const char *lib_name, int flags) {
  ALOGD("dlopen %s", lib_name);
  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  pthread_once(&once_control, init_api);
  if (android_api_ < __ANDROID_API_N__) {
    return ::dlopen(lib_name, flags);
  }
  if (android_api_ > __ANDROID_API_N__) {
    void *handle = ::dlopen("libdl.so", flags);
    CHECKP(handle)
    auto __loader_dlopen = reinterpret_cast<__loader_dlopen_fn>(::dlsym(handle, "__loader_dlopen"));
    CHECKP(__loader_dlopen)
    if (android_api_ < __ANDROID_API_Q__) {
      return __loader_dlopen(lib_name, flags, (void *)dlerror);
    } else {
      handle = __loader_dlopen(lib_name, flags, (void *)dlerror);
      if (handle == nullptr) {
        // Support more namespaces(e.g. runtime)
        dl_iterate_data data{};
        data.info_.dlpi_name = lib_name;
        dl_iterate_phdr_wrapper(dl_iterate_callback, &data);
        CHECKP(data.info_.dlpi_addr > 0)
        handle = __loader_dlopen(lib_name, flags, (void *)data.info_.dlpi_addr);
      }
      return handle;
    }
  }
  // __ANDROID_API_N__
  auto *data = new dl_iterate_data();
  data->info_.dlpi_name = lib_name;
  dl_iterate_phdr_wrapper(dl_iterate_callback, data);

  return data;
}

KWAI_EXPORT void *DlFcn::dlsym(void *handle, const char *name) {
  ALOGD("dlsym %s", name);
  CHECKP(handle)
  if (android_api_ != __ANDROID_API_N__) {
    return ::dlsym(handle, name);
  }
  // __ANDROID_API_N__
  auto *data = (dl_iterate_data *)handle;
  ElfW(Addr) dlpi_addr = data->info_.dlpi_addr;
  const char *dlpi_name = data->info_.dlpi_name;
  const ElfW(Phdr) *dlpi_phdr = data->info_.dlpi_phdr;
  ElfW(Half) dlpi_phnum = data->info_.dlpi_phnum;
  // preserved for parse .symtab
  ElfW(Addr) elf_base_addr;

  for (int i = 0; i < dlpi_phnum; ++i) {
    if (dlpi_phdr[i].p_type == PT_LOAD && dlpi_phdr[i].p_offset == 0) {
      elf_base_addr = dlpi_addr + dlpi_phdr[i].p_vaddr;
      ALOGD("PT_LOAD dlpi_addr %p p_vaddr %p elf_base_addr %p", dlpi_addr, dlpi_phdr[i].p_vaddr,
            elf_base_addr);
    }
    if (dlpi_phdr[i].p_type == PT_DYNAMIC) {
      ElfW(Dyn) *dyn = (ElfW(Dyn) *)(dlpi_addr + dlpi_phdr[i].p_vaddr);
      ElfW(Dyn) *dyn_end = dyn + (dlpi_phdr[i].p_memsz / sizeof(ElfW(Dyn)));
      const char *strtab;
      ElfW(Sym) * symtab;
      bool is_use_gnu_hash = false;
      // for ELF hash
      size_t nbucket_;
      size_t nchain_;
      uint32_t *bucket_;
      uint32_t *chain_;
      // for GNU hash
      size_t gnu_nbucket_;
      uint32_t *gnu_bucket_;
      uint32_t *gnu_chain_;
      uint32_t gnu_maskwords_;
      uint32_t gnu_shift2_;
      ElfW(Addr) * gnu_bloom_filter_;
      // ELF parse
      for (; dyn < dyn_end; dyn++) {
        switch (dyn->d_tag) {
        case DT_NULL:
          // the end of the dynamic-section
          dyn = dyn_end;
          break;
        case DT_STRTAB: {
          ElfW(Addr) strtab_addr = dlpi_addr + dyn->d_un.d_ptr;
          strtab = reinterpret_cast<const char *>(strtab_addr);
          CHECKP(strtab_addr >= dlpi_addr)
          break;
        }
        case DT_SYMTAB: {
          ElfW(Addr) symtab_addr = dlpi_addr + dyn->d_un.d_ptr;
          symtab = reinterpret_cast<ElfW(Sym) *>(symtab_addr);
          CHECKP(symtab_addr >= dlpi_addr)
          break;
        }
        case DT_HASH: {
          // ignore DT_HASH when ELF contains DT_GNU_HASH hash table
          if (is_use_gnu_hash) {
            continue;
          }
          nbucket_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr)[0];
          nchain_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr)[1];
          bucket_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr + 8);
          chain_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr + 8 + nbucket_ * 4);
          break;
        }
        case DT_GNU_HASH: {
          gnu_nbucket_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr)[0];
          // skip symndx
          gnu_maskwords_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr)[2];
          gnu_shift2_ = reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr)[3];

          gnu_bloom_filter_ = reinterpret_cast<ElfW(Addr) *>(dlpi_addr + dyn->d_un.d_ptr + 16);
          gnu_bucket_ = reinterpret_cast<uint32_t *>(gnu_bloom_filter_ + gnu_maskwords_);
          // amend chain for symndx = header[1]
          gnu_chain_ = gnu_bucket_ + gnu_nbucket_ -
                       reinterpret_cast<uint32_t *>(dlpi_addr + dyn->d_un.d_ptr)[1];
          --gnu_maskwords_;

          is_use_gnu_hash = true;
          break;
        }
        default:
          break;
        }
      }
      // lookup symbol
      if (is_use_gnu_hash) {
        ALOGD("lookup use gnu hash");
        uint32_t hash = elf_gnu_hash((uint8_t *)name);
        constexpr uint32_t kBloomMaskBits = sizeof(ElfW(Addr)) * 8;
        const uint32_t word_num = (hash / kBloomMaskBits) & gnu_maskwords_;
        const ElfW(Addr) bloom_word = gnu_bloom_filter_[word_num];
        const uint32_t h1 = hash % kBloomMaskBits;
        const uint32_t h2 = (hash >> gnu_shift2_) % kBloomMaskBits;
        // test against bloom filter
        CHECKP((1 & (bloom_word >> h1) & (bloom_word >> h2)) != 0)
        // bloom test says "probably yes"...
        uint32_t n = gnu_bucket_[hash % gnu_nbucket_];

        do {
          ElfW(Sym) *s = symtab + n;
          if (((gnu_chain_[n] ^ hash) >> 1) == 0 && strcmp(strtab + s->st_name, name) == 0) {
            ALOGD("find %s %p", name, dlpi_addr + s->st_value);
            return reinterpret_cast<void *>(dlpi_addr + s->st_value);
          }
        } while ((gnu_chain_[n++] & 1) == 0);
      } else {
        ALOGD("lookup use elf hash");
        uint32_t hash = elf_hash((uint8_t *)name);
        for (uint32_t n = bucket_[hash % nbucket_]; n != 0; n = chain_[n]) {
          ElfW(Sym) *s = symtab + n;
          if (strcmp(strtab + s->st_name, name) == 0) {
            ALOGD("find %s %p", name, dlpi_addr + s->st_value);
            return reinterpret_cast<void *>(dlpi_addr + s->st_value);
          }
        }
      }
    }
  }
  return nullptr;
}

KWAI_EXPORT int DlFcn::dlclose(void *handle) {
  if (android_api_ != __ANDROID_API_N__) {
    return ::dlclose(handle);
  }
  // __ANDROID_API_N__
  delete (dl_iterate_data *)handle;
  return 0;
}

uint32_t DlFcn::elf_gnu_hash(const uint8_t *name) {
  uint32_t h = 5381;

  while (*name != 0) {
    h += (h << 5) + *name++; // h*33 + c = h + h * 32 + c = h + h << 5 + c
  }
  return h;
}

uint32_t DlFcn::elf_hash(const uint8_t *name) {
  uint32_t h = 0, g;

  while (*name) {
    h = (h << 4) + *name++;
    g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
  }

  return h;
}

struct ctx {
  uint8_t *load_addr;
  void *dynstr;
  void *dynsym;
  void *symtab;
  void *strtab;
  int dynsym_num;
  int symtab_num;
  off_t bias;
};

KWAI_EXPORT void *DlFcn::dlopen_elf(const char *lib_name, int flags) {
  dl_iterate_data data{};
  data.info_.dlpi_name = lib_name;
  if (!dl_iterate_phdr) {
    return nullptr;
  }
  dl_iterate_phdr(dl_iterate_callback, &data);
  const char *lib_path = data.info_.dlpi_name;
  ElfW(Addr) dlpi_addr = data.info_.dlpi_addr;
  const ElfW(Phdr) *dlpi_phdr = data.info_.dlpi_phdr;
  ElfW(Half) dlpi_phnum = data.info_.dlpi_phnum;
  // preserved for parse .symtab
  ElfW(Addr) elf_base_addr;

  for (int i = 0; i < dlpi_phnum; ++i) {
    if (dlpi_phdr[i].p_type == PT_LOAD && dlpi_phdr[i].p_offset == 0) {
      elf_base_addr = dlpi_addr + dlpi_phdr[i].p_vaddr;
      ALOGD("PT_LOAD dlpi_addr %p p_vaddr %p elf_base_addr %p", dlpi_addr, dlpi_phdr[i].p_vaddr,
            elf_base_addr);
    }
  }

  struct ctx *ctx = 0;
  off_t size;
  int k, fd = -1;
  uint8_t *shoff;
  auto *elf = reinterpret_cast<ElfW(Ehdr) *>(MAP_FAILED);
  bool bias_flag;
  ElfW(Shdr) * shstrtab;
  char *shstr;

#define fatal(fmt, args...)                                                                        \
  do {                                                                                             \
    ALOGE(fmt, ##args);                                                                            \
    goto err_exit;                                                                                 \
  } while (0)

  /* Now, mmap the same library once again */
  fd = open(lib_path, O_RDONLY);
  if (fd < 0)
    fatal("failed to open %s", lib_path);

  size = lseek(fd, 0, SEEK_END);
  if (size <= 0)
    fatal("lseek() failed for %s", lib_path);

  elf = reinterpret_cast<ElfW(Ehdr) *>(mmap(0, size, PROT_READ, MAP_SHARED, fd, 0));
  close(fd);
  fd = -1;

  if (elf == MAP_FAILED)
    fatal("mmap() failed for %s", lib_path);

  ctx = (struct ctx *)calloc(1, sizeof(struct ctx));
  if (!ctx)
    fatal("no memory for %s", lib_path);

  ctx->load_addr = reinterpret_cast<uint8_t *>(elf_base_addr);
  shoff = reinterpret_cast<uint8_t *>(elf) + elf->e_shoff;

  shstrtab = (ElfW(Shdr) *)(shoff + elf->e_shstrndx * elf->e_shentsize);
  shstr = static_cast<char *>(malloc(shstrtab->sh_size));
  memcpy(shstr, reinterpret_cast<uint8_t *>(elf) + shstrtab->sh_offset, shstrtab->sh_size);

  bias_flag = false;
  for (k = 0; k < elf->e_shnum; k++, shoff += elf->e_shentsize) {

    ElfW(Shdr) *sh = (ElfW(Shdr) *)shoff;
    ALOGD("%s: k=%d shdr=%p type=%x", __func__, k, sh, sh->sh_type);

    switch (sh->sh_type) {

    case SHT_DYNSYM:
      if (ctx->dynsym)
        fatal("%s: duplicate DYNSYM sections", lib_path); /* .dynsym */
      ctx->dynsym = malloc(sh->sh_size);
      if (!ctx->dynsym)
        fatal("%s: no memory for .dynsym", lib_path);
      memcpy(ctx->dynsym, reinterpret_cast<uint8_t *>(elf) + sh->sh_offset, sh->sh_size);
      ctx->dynsym_num = (sh->sh_size / sizeof(ElfW(Sym)));
      break;

    case SHT_SYMTAB:
      if (ctx->symtab)
        fatal("%s: duplicate SYMTAB sections", lib_path); /* .symtab */
      ctx->symtab = malloc(sh->sh_size);
      if (!ctx->symtab)
        fatal("%s: no memory for .symtab", lib_path);
      memcpy(ctx->symtab, reinterpret_cast<uint8_t *>(elf) + sh->sh_offset, sh->sh_size);
      ctx->symtab_num = (sh->sh_size / sizeof(ElfW(Sym)));
      break;

    case SHT_STRTAB:
      if (!strcmp(shstr + sh->sh_name, ".dynstr")) {
        if (ctx->dynstr)
          break; /* .dynstr is guaranteed to be the first STRTAB */
        ctx->dynstr = malloc(sh->sh_size);
        if (!ctx->dynstr)
          fatal("%s: no memory for .dynstr", lib_path);
        memcpy(ctx->dynstr, reinterpret_cast<uint8_t *>(elf) + sh->sh_offset, sh->sh_size);
      } else if (!strcmp(shstr + sh->sh_name, ".strtab")) {
        if (ctx->strtab)
          break;
        ctx->strtab = malloc(sh->sh_size);
        if (!ctx->strtab)
          fatal("%s: no memory for .strtab", lib_path);
        memcpy(ctx->strtab, reinterpret_cast<uint8_t *>(elf) + sh->sh_offset, sh->sh_size);
      }
      break;

    case SHT_PROGBITS:
      if (!ctx->dynstr || !ctx->dynsym || bias_flag)
        break;
      /* won't even bother checking against the section name */
      ctx->bias = (off_t)sh->sh_addr - (off_t)sh->sh_offset;
      bias_flag = true;
      break;
    }
  }

  munmap(elf, size);
  elf = 0;

  if (!ctx->dynstr || !ctx->dynsym)
    fatal("dynamic sections not found in %s", lib_path);

  ALOGD("%s: ok, dynsym = %p, dynstr = %p", lib_path, ctx->dynsym, ctx->dynstr);

  return ctx;

err_exit:
  if (fd >= 0)
    close(fd);
  if (elf != MAP_FAILED)
    munmap(elf, size);
  dlclose_elf(ctx);
  return 0;
}

KWAI_EXPORT void *DlFcn::dlsym_elf(void *handle, const char *name) {
  CHECKP(handle)
  int k;
  auto *ctx = (struct ctx *)handle;
  auto *dynsym = (ElfW(Sym) *)ctx->dynsym;
  auto *symtab = (ElfW(Sym) *)ctx->symtab;
  char *dynstr = (char *)ctx->dynstr;
  char *strtab = (char *)ctx->strtab;

  // search in .dynsym
  for (k = 0; k < ctx->dynsym_num; k++, dynsym++)
    if (strcmp(dynstr + dynsym->st_name, name) == 0) {
      /*  NB: dynsym->st_value is an offset into the section for relocatables,
    but a VMA for shared libs or exe files, so we have to subtract the bias */
      void *ret = ctx->load_addr + dynsym->st_value - ctx->bias;
      ALOGI("%s found at %p", name, ret);
      return ret;
    }

  // search in .symtab
  if (symtab) {
    for (k = 0; k < ctx->symtab_num; k++, symtab++) {
      if (strcmp(strtab + symtab->st_name, name) == 0) {
        void *ret = ctx->load_addr + symtab->st_value - ctx->bias;
        ALOGI("%s found at %p", name, ret);
        return ret;
      }
    }
  }

  return 0;
}

KWAI_EXPORT int DlFcn::dlclose_elf(void *handle) {
  CHECKI(handle)
  auto *ctx = (struct ctx *)handle;
  if (ctx->dynsym)
    free(ctx->dynsym);
  if (ctx->dynstr)
    free(ctx->dynstr);
  if (ctx->dynstr)
    free(ctx->symtab);
  if (ctx->dynstr)
    free(ctx->strtab);
  free(ctx);
  return 0;
}

} // namespace linker

} // namespace kwai
