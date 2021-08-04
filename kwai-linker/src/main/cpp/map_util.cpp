//
// Created by wanglianbao on 2021/8/3.
//
#include <link.h>
#include <android/log.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <string>
#include <vector>

namespace kwai {
namespace linker {
class MapUtil {
 public:
  static bool GetLoadInfo(const std::string &name, ElfW(Addr) *load_base,
  std::string &so_full_name, int android_api) {
    auto get_load_info = android_api > __ANDROID_API_O__ ?
        GetLoadInfoAboveAPIO : GetLoadInfoBelowAPIO;
    return get_load_info(name, load_base, so_full_name);
  }
 private:
  typedef struct {
    std::string name;
    uintptr_t start;
    uintptr_t end;
    uintptr_t offset;
    int flags;
  } MapEntry;

  template<typename T>
  static inline bool GetVal(MapEntry &entry, uintptr_t addr, T *store) {
    if (!(entry.flags & PROT_READ) || addr < entry.start || addr + sizeof(T) > entry.end) {
      return false;
    }
    // Make sure the address is aligned properly.
    if (addr & (sizeof(T) - 1)) {
      return false;
    }
    *store = *reinterpret_cast<T *>(addr);
    return true;
  }

  static bool ReadLoadBias(MapEntry &entry, ElfW(Addr) *load_bias) {
    uintptr_t addr = entry.start;
    ElfW(Ehdr) ehdr;
    if (!GetVal<ElfW(Half)>(entry, addr + offsetof(ElfW(Ehdr), e_phnum), &ehdr.e_phnum)) {
      return false;
    }
    if (!GetVal<ElfW(Off)>(entry, addr + offsetof(ElfW(Ehdr), e_phoff), &ehdr.e_phoff)) {
      return false;
    }
    addr += ehdr.e_phoff;
    for (size_t i = 0; i < ehdr.e_phnum; i++) {
      ElfW(Phdr) phdr;
      if (!GetVal<ElfW(Word)>(entry, addr + offsetof(ElfW(Phdr), p_type), &phdr.p_type)) {
        return false;
      }
      if (!GetVal<ElfW(Word)>(entry, addr + offsetof(ElfW(Phdr), p_flags), &phdr.p_flags)) {
        return false;
      }
      if (!GetVal<ElfW(Off)>(entry, addr + offsetof(ElfW(Phdr), p_offset), &phdr.p_offset)) {
        return false;
      }

      if ((phdr.p_type == PT_LOAD) && (phdr.p_flags & PF_X)) {
        if (!GetVal<ElfW(Addr)>(entry, addr + offsetof(ElfW(Phdr), p_vaddr), &phdr.p_vaddr)) {
          return false;
        }
        if (phdr.p_offset != phdr.p_vaddr) {
          __android_log_print(ANDROID_LOG_INFO, "map", "so %s diff offset %x vddr %x", entry.name
                                  .c_str(),
                              phdr
                                  .p_offset,
                              phdr
                                  .p_vaddr);
        }
        *load_bias = phdr.p_vaddr;
        return true;
      }
      addr += sizeof(phdr);
    }
  }

  static bool EndsWith(const char *target, const char *suffix) {
    if (!target || !suffix) {
      return false;
    }
    const char *sub_str = strstr(target, suffix);
    return sub_str && strlen(sub_str) == strlen(suffix);
  }

  static bool GetLoadInfoBelowAPIO(const std::string &name, ElfW(Addr) *load_base,
                                   std::string &full_name) {
    FILE *fp = fopen("/proc/self/maps", "re");
    auto ret = false;
    if (fp == nullptr) {
      return ret;
    }

    auto parse_line = [](char *map_line, MapEntry &curr_entry, int &name_pos) -> bool {
      char permissions[5];
      if (sscanf(map_line, "%" PRIxPTR "-%" PRIxPTR " %4s %" PRIxPTR " %*x:%*x %*d %n",
        &curr_entry.start, &curr_entry.end, permissions, &curr_entry.offset, &name_pos) < 4) {
      return false;
    }
      curr_entry.flags = 0;
      if (permissions[0] == 'r') {
        curr_entry.flags |= PROT_READ;
      }
      if (permissions[2] == 'x') {
        curr_entry.flags |= PROT_EXEC;
      }
      return true;
    };
    std::vector<char> buffer(1024);
    MapEntry prev_entry = {};
    while (fgets(buffer.data(), buffer.size(), fp) != nullptr) {
      MapEntry curr_entry = {};
      int name_pos;

      if (!parse_line(buffer.data(), curr_entry, name_pos)) {
        continue;
      }

      const char *map_name = buffer.data() + name_pos;
      size_t name_len = strlen(map_name);
      if (name_len && map_name[name_len - 1] == '\n') {
        name_len -= 1;
      }

      curr_entry.name = std::string(map_name, name_len);
      if (curr_entry.flags == PROT_NONE) {
        continue;
      }

      if ((curr_entry.flags & PROT_EXEC) == PROT_EXEC &&
          EndsWith(curr_entry.name.c_str(), name.c_str())) {
        ElfW(Addr) load_bias;
        if (curr_entry.offset == 0) {
          ret = ReadLoadBias(curr_entry, &load_bias);
        } else {
          if (EndsWith(prev_entry.name.c_str(), name.c_str()) && prev_entry.offset == 0 &&
              prev_entry.flags == PROT_READ) {
            ret = ReadLoadBias(prev_entry, &load_bias);
          }
        }

        if (ret) {
          *load_base = curr_entry.start - load_bias;
          full_name = curr_entry.name;
          break;
        }
      }
      prev_entry = curr_entry;
    }
    fclose(fp);
    return ret;
  }

  static bool GetLoadInfoAboveAPIO(const std::string &name, ElfW(Addr) *load_base,
                                   std::string &so_full_name) {
    typedef struct {
      const char *name;
      std::string full_name;
      ElfW(Addr) load_base;
      off_t load_bias;
    } PhdrInfo;
    PhdrInfo phdr_info = {
        .name = name.c_str(),
        .full_name = "",
        .load_base = 0
    };
    auto iterate_phdr_callback = [](struct dl_phdr_info* phdr_info, size_t size, void* data) -> int {
      PhdrInfo *info = reinterpret_cast<PhdrInfo *>(data);
      if (!phdr_info->dlpi_name) {
        __android_log_print(ANDROID_LOG_INFO, "map", "name nullptr");
        return 0;
      }
      const char *sub_str = strstr(phdr_info->dlpi_name, info->name);
      if (sub_str && strlen(sub_str) == strlen(info->name)) {
        info->load_base = phdr_info->dlpi_addr;
        info->full_name = phdr_info->dlpi_name;
        return 1;
      }
      return 0;
    };

    dl_iterate_phdr(iterate_phdr_callback, reinterpret_cast<void *>(&phdr_info));
    if (!phdr_info.load_base) {
      return false;
    }

    *load_base = phdr_info.load_base;
    so_full_name = phdr_info.full_name;
    return true;
  }
};
} // namespace kwai
} // namespace linker

