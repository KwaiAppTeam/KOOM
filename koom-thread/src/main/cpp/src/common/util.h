//
// Created by lirui on 2020/11/3.
//

#ifndef APM_UTIL_H
#define APM_UTIL_H

#include <string>
#include <set>
#include "log.h"
#include <vector>
#include <map>
#include <fstream>
#include <streambuf>
#include <dirent.h>
#include <jni.h>

namespace koom {

class Util {
//private:
  //  static int android_api;

 public:
  static int android_api;

  static void Init() {
    android_api = android_get_device_api_level();
  }

  static int AndroidApi() {
    return android_api;
  }

  static timespec CurrentClockTime() {
    struct timespec now_time{};
    clock_gettime(CLOCK_MONOTONIC, &now_time);
    return now_time;
  }

  static long long CurrentTimeNs() {
    struct timespec now_time{};
    clock_gettime(CLOCK_MONOTONIC, &now_time);
    return now_time.tv_sec * 1000000000LL + now_time.tv_nsec;
  }

  static std::vector<std::string> Split(const std::string &s, char seperator) {
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find(seperator, pos)) != std::string::npos) {
      std::string substring(s.substr(prev_pos, pos - prev_pos));
      output.push_back(substring);
      prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word

    return output;
  }

  static std::map<int, std::string> getProcessThreadList() { // tid,name
    std::map<int, std::string> result;
    char *path = "/proc/self/task";
    try {
      struct dirent *entry;
      DIR *dir = opendir(path);
      while ((entry = readdir(dir)) != nullptr) {
        auto tid = std::string(entry->d_name);
        if (tid.rfind(".", 0) == 0) {
          // 过滤掉.开头的文件
          continue;
        }
        auto comm = std::string(path) + std::string("/") + tid +
            std::string("/comm");
        std::ifstream t(comm.c_str());
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        auto p = str.find_last_of("\n");
        auto key = std::stoi(tid);
        if (p == str.npos) {
          result[key] = str;
        } else {
          result[key] = str.substr(0, p);
        }
      }
      closedir(dir);
    } catch (int e) {}
    return result;
  };
  static std::set<std::string> GetProcessSoList() {
    std::set<std::string> soSet;
    FILE *fp;
    char buffer[512];

    if (nullptr == (fp = fopen("/proc/self/maps", "r"))) {
      return soSet;
    }

    while (fgets(buffer, sizeof(buffer), fp)) {
      const char *sep = "\t \r\n";
      char *line = nullptr;
      char *addr = strtok_r(buffer, sep, &line);
      if (!addr) {
        continue;
      }

      char *flags = strtok_r(nullptr, sep, &line);
      if (!flags || flags[0] != 'r' || flags[3] == 's') {

        //log_info("######## FIRST CRASHING IF ********************");
        //
        /*
            1. mem section cound NOT be read, without 'r' flag.
            2. read from base addr of /dev/mail module would crash.
               i dont know how to handle it, just skip it.

               1f5573000-1f58f7000 rw-s 1f5573000 00:0c 6287 /dev/mali0

        */
        continue;
      }
      strtok_r(nullptr, sep, &line);  // offsets
      //char* dev =
      strtok_r(nullptr, sep, &line);  // dev number.

      // int major = 0, minor = 0;
      // if (!phrase_dev_num(dev, &major, &minor) || major == 0) {

      //log_info("######## SECOND CRASHING IF ********************");
      /*
          if dev major number equal to 0, mean the module must NOT be
          a shared or executable object loaded from disk.
          e.g:
          lookup symbol from [vdso] would crash.
          7f7b48a000-7f7b48c000 r-xp 00000000 00:00 0  [vdso]
      */
      //    continue;
      //}


      strtok_r(nullptr, sep, &line);  // node

      char *filename = strtok_r(nullptr, sep, &line); //module name
      if (!filename) {
        continue;
      }
      int name_len = strlen(filename);
      if (name_len >= 10 && strcmp(filename + name_len - 3, ".so") == 0) {
        soSet.insert(std::string(filename));
      }
    }
    fclose(fp);
    return soSet;
  }
};
}

#endif //APM_UTIL_H
