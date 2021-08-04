//
// Created by wanglianbao on 2021/8/3.
//

#include "include/elf_reader.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <kwai_log.h>
#include <string.h>

namespace kwai {
namespace linker {
ElfReader::ElfReader(const char *name) {
  if (!name) {
    return;
  }
  fd_ = open(name, O_RDONLY);
  if (fd_ < 0) {
    return;
  }
  size_ = lseek(fd_, 0, SEEK_END);
  if (size_ <= 0)
    return;

  start_ = reinterpret_cast<ElfW(Ehdr) *>(mmap(0, size_, PROT_READ, MAP_SHARED, fd_, 0));
  if (start_ == MAP_FAILED) {
    return;
  }
}

bool ElfReader::IsValidElf() {
  return fd_ >= 0 && size_ > 0 && start_ != MAP_FAILED &&
      !memcmp(start_->e_ident, ELFMAG, SELFMAG);
}

bool ElfReader::Init() {
  if (!IsValidElf() || !IsValidRange(start_->e_ehsize)) {
    return false;
  }

  phdr_table_ = SafelyOffset<ElfW(Phdr)>(start_->e_phoff);
  shdr_table_ = SafelyOffset<ElfW(Shdr)>(start_->e_shoff);
  if (!phdr_table_ || !shdr_table_) {
    return false;
  }
  const char *shstr = SafelyOffset<const char>(shdr_table_[start_->e_shstrndx].sh_offset);
  if (!shstr) {
    return false;
  }
  for (int index = 0; index < start_->e_shnum; ++index) {
    switch (shdr_table_[index].sh_type) {
      case SHT_DYNSYM:
        dynsym_ = SafelyOffset<ElfW(Sym)>(shdr_table_[index].sh_offset);
        break;
      case SHT_STRTAB:
        if (!strncmp(shstr + shdr_table_[index].sh_name, ".dynstr", sizeof(".dynstr"))) {
          dynstr_ = SafelyOffset<const char>(shdr_table_[index].sh_offset);
        } else if (!strncmp(shstr + shdr_table_[index].sh_name, ".strtab", sizeof(".strtab"))) {
          strtab_ = SafelyOffset<const char>(shdr_table_[index].sh_offset);
        }
        break;
      case SHT_SYMTAB:
        symtab_ = SafelyOffset<ElfW(Sym)>(shdr_table_[index].sh_offset);
        symtab_entsize_ = shdr_table_[index].sh_entsize;
        break;
      case SHT_HASH:
        IsValidRange(shdr_table_[index].sh_offset + shdr_table_[index].sh_size);
        BuildHash(SafelyOffset<ElfW(Word)>(shdr_table_[index].sh_offset));
        break;
      case SHT_PROGBITS:
        if (!strncmp(shstr + shdr_table_[index].sh_name, ".gnu_debugdata",
                     sizeof(".gnu_debugdata"))) {
          gnu_debugdata_ = SafelyOffset<const char>(shdr_table_[index].sh_offset);
          gnu_debugdata_size_ = shdr_table_[index].sh_size;
        }
      default:
        if (!strncmp(shstr + shdr_table_[index].sh_name, ".gnu.hash", sizeof(".gnu.hash"))) {
          BuildGnuHash(SafelyOffset<ElfW(Word)>(shdr_table_[index].sh_offset));
        }
        break;
    }
  }
  return true;
}

void *ElfReader::LookupSymbol(const char *symbol, ElfW(Addr) load_base, bool only_dynsym) {
  if (!symbol) {
    return nullptr;
  }

  // First lookup from dynsym using hash
  ElfW(Addr) sym_vaddr = has_gnu_hash_ ? LookupByGnuHash(symbol) : LookupByElfHash(symbol);
  if (sym_vaddr != 0) {
    return reinterpret_cast<void *>(load_base + sym_vaddr);
  }

  if (only_dynsym) {
    return nullptr;
  }

  // Try lookup from symtab
  for (int index = 0; index < symtab_entsize_; index++) {
    if (!strncmp(strtab_ + symtab_[index].st_name, symbol, sizeof(symbol))) {
      return reinterpret_cast<void *>(load_base + symtab_[index].st_value);
    }
  }

  // Try lookup from compressed gnu_debugdata
  return nullptr;
}

ElfReader::~ElfReader() {
  if (start_ != MAP_FAILED && size_ > 0) {
    munmap(reinterpret_cast<void *>(start_), size_);
  }
  if (fd_ >= 0) {
    close(fd_);
  }
}

template<class T> T * ElfReader::SafelyOffset(off_t offset) {
  if (!IsValidRange(offset)) {
    return nullptr;
  }
  return reinterpret_cast<T *>(reinterpret_cast<ElfW(Addr)>(start_) +
      offset);
}

bool ElfReader::IsValidRange(off_t offset) {
  return offset < size_;
}

void ElfReader::BuildHash(ElfW(Word) *hash_section) {
  if (!hash_section) {
    return;
  }

  elf_hash_.nbucket = hash_section[0];
  elf_hash_.nchain = hash_section[1];
  elf_hash_.bucket = hash_section + 2;
  elf_hash_.chain = hash_section + 2 + elf_hash_.nbucket;
  has_elf_hash_ = true;
}

void ElfReader::BuildGnuHash(ElfW(Word) *gnu_hash_section) {
  if (!gnu_hash_section) {
    return;
  }

  gnu_hash_.gnu_nbucket = gnu_hash_section[0];
  gnu_hash_.gnu_maskwords = gnu_hash_section[2];
  gnu_hash_.gnu_shift2 = gnu_hash_section[3];
  gnu_hash_.gnu_bloom_filter = reinterpret_cast<ElfW(Addr) *>(gnu_hash_section + 4);
  gnu_hash_.gnu_bucket =
      reinterpret_cast<ElfW(Word) *>(gnu_hash_.gnu_bloom_filter + gnu_hash_.gnu_maskwords);
  gnu_hash_.gnu_chain =
      gnu_hash_.gnu_bucket + gnu_hash_.gnu_nbucket - gnu_hash_section[1];
  gnu_hash_.gnu_maskwords--;
  has_gnu_hash_ = true;
}

// Hash Only search dynsym and we ignore symbol version check
ElfW(Addr) ElfReader::LookupByElfHash(const char *symbol) {
  if (!has_elf_hash_ || !dynsym_ || !dynstr_) {
    return 0;
  }
  uint32_t hash = elf_hash_.Hash(reinterpret_cast<const uint8_t *>(symbol));
  for (uint32_t n = elf_hash_.bucket[hash % elf_hash_.nbucket]; n != 0; n = elf_hash_.chain[n]) {
    const ElfW(Sym) *sym = dynsym_ + n;
    if (strcmp(dynstr_ + sym->st_name, symbol) == 0) {
      // TODO add log
      return sym->st_value;
    }
  }
  return 0;
}

// Gnu hash Only search dynsym and we ignore symbol version check
ElfW(Addr) ElfReader::LookupByGnuHash(const char *symbol) {
  if (!has_gnu_hash_ || !dynsym_ || !dynstr_) {
    return 0;
  }

  uint32_t hash = gnu_hash_.Hash(reinterpret_cast<const uint8_t *>(symbol));
  constexpr uint32_t kBloomMaskBits = sizeof(ElfW(Addr)) * 8;
  const uint32_t word_num = (hash / kBloomMaskBits) & gnu_hash_.gnu_maskwords;
  const ElfW(Addr) bloom_word = gnu_hash_.gnu_bloom_filter[word_num];
  const uint32_t h1 = hash % kBloomMaskBits;
  const uint32_t h2 = (hash >> gnu_hash_.gnu_shift2) % kBloomMaskBits;
  // test against bloom filter
  if ((1 & (bloom_word >> h1) & (bloom_word >> h2)) == 0) {
    return 0;
  }
  // bloom test says "probably yes"...
  uint32_t n = gnu_hash_.gnu_bucket[hash % gnu_hash_.gnu_nbucket];

  do {
    const ElfW(Sym) *sym = dynsym_ + n;
    if (((gnu_hash_.gnu_chain[n] ^ hash) >> 1) == 0 && strcmp(dynstr_ + sym->st_name, symbol) == 0) {
      return sym->st_value;
    }
  } while ((gnu_hash_.gnu_chain[n++] & 1) == 0);
  return 0;
}
} // namespace linker
} // namespace kwai