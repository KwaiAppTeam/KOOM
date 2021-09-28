// Copyright 2020 Kwai, Inc. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//         http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: lbtrace

#ifndef KOOM_KWAI_LINKER_SRC_MAIN_CPP_INCLUDE_ELF_READER_H_
#define KOOM_KWAI_LINKER_SRC_MAIN_CPP_INCLUDE_ELF_READER_H_

#include <link.h>
#include <string>
#include "elf_wrapper.h"

namespace kwai {
namespace linker {
class ElfReader {
 public:
  struct ElfHash {
    ElfW(Word) nbucket;
    ElfW(Word) nchain;
    ElfW(Word)* bucket;
    ElfW(Word)* chain;
    uint32_t Hash(const uint8_t *name) {
      uint32_t h = 0, g;

      while (*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        h ^= g;
        h ^= g >> 24;
      }

      return h;
    }
  };

  struct GnuHash {
    ElfW(Word) gnu_nbucket;
    ElfW(Word) gnu_maskwords;
    ElfW(Word) gnu_shift2;
    ElfW(Addr)* gnu_bloom_filter;
    ElfW(Word)* gnu_bucket;
    ElfW(Word)* gnu_chain;
    uint32_t Hash(const uint8_t *name) {
      uint32_t h = 5381;

      while (*name != 0) {
        h += (h << 5) + *name++; // h*33 + c = h + h * 32 + c = h + h << 5 + c
      }
      return h;
    }
  };

  explicit ElfReader(std::shared_ptr<ElfWrapper> elf_wrapper);
  bool IsValidElf();
  bool Init();
  /**
   * Lookup symbol address(load_base + symbol vaddr) from the ELF file, if fail return nullptr
   *
   * 1. Lookup symbol from dynsym table using hash/gnu_hash
   * 2. Try read symtab(symtab NOT in loaded segments) from ELF, then linear lookup symbol
   * 3. Try read gnu_debugdata(lZMA compressed ELF) from ELF, then linear lookup symtab
   */
  void *LookupSymbol(const char *symbol, ElfW(Addr) load_base, bool only_dynsym = false);
  ~ElfReader() = default;

 private:
  template<class T>T *CheckedOffset(off_t offset, size_t size);
  bool IsValidRange(off_t offset);
  void BuildHash(ElfW(Word) *hash_section);
  void BuildGnuHash(ElfW(Word) *gnu_hash_section);
  ElfW(Addr) LookupByElfHash(const char *symbol);
  ElfW(Addr) LookupByGnuHash(const char *symbol);
  bool DecGnuDebugdata(std::string &decompressed_data);
  std::shared_ptr<ElfWrapper> elf_wrapper_;
  const ElfW(Shdr)* shdr_table_;
  const ElfW(Sym)* dynsym_;
  const char *dynstr_;
  const ElfW(Sym)* symtab_;
  ElfW(Word) symtab_ent_count_;
  const char *strtab_;
  const char *gnu_debugdata_;
  ElfW(Word) gnu_debugdata_size_;
  ElfHash elf_hash_;
  bool has_elf_hash_;
  GnuHash gnu_hash_;
  bool has_gnu_hash_;
};
} // namespace linker
} // namespace kwai
#endif // KOOM_KWAI_LINKER_SRC_MAIN_CPP_INCLUDE_ELF_READER_H_
