//
// Created by wanglianbao on 2021/8/3.
//

#ifndef KOOM_KWAI_LINKER_SRC_MAIN_CPP_INCLUDE_ELF_READER_H_
#define KOOM_KWAI_LINKER_SRC_MAIN_CPP_INCLUDE_ELF_READER_H_

#include <link.h>

namespace kwai {
namespace linker {
class ElfReader {
 public:
  typedef struct {
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
  } ElfHash;

  typedef struct {
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
  } GnuHash;

  ElfReader(const char *name);
  bool IsValidElf();
  bool Init();
  void *LookupSymbol(const char *symbol, ElfW(Addr) load_base, bool only_dynsym = false);
  ~ElfReader();

 private:
  template<class T>T *SafelyOffset(off_t offset);
  bool IsValidRange(off_t offset);
  void BuildHash(ElfW(Word) *hash_section);
  void BuildGnuHash(ElfW(Word) *gnu_hash_section);
  ElfW(Addr) LookupByElfHash(const char *symbol);
  ElfW(Addr) LookupByGnuHash(const char *symbol);
  int fd_ = -1;
  size_t size_ = 0;
  ElfW(Ehdr) *start_ = nullptr;
  const ElfW(Phdr)* phdr_table_;
  ElfW(Half) phdr_num_;
  const ElfW(Shdr)* shdr_table_;
  const ElfW(Sym)* dynsym_;
  const char *dynstr_;
  const ElfW(Sym)* symtab_;
  ElfW(Word) symtab_entsize_;
  const char *strtab_;
  const char *gnu_debugdata_;
  ElfW(Word) gnu_debugdata_size_;
  ElfHash elf_hash_;
  bool has_elf_hash_ = false;
  GnuHash gnu_hash_;
  bool has_gnu_hash_ = false;
};
} // namespace linker
} // namespace kwai
#endif //KOOM_KWAI_LINKER_SRC_MAIN_CPP_INCLUDE_ELF_READER_H_
