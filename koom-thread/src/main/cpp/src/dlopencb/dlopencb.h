//
// Created by shenguanchu.
//

#pragma once

#include <cstddef>
#include <string>
#include <set>
#include <android/dlext.h>
#include <kwai_linker/kwai_dlfcn.h>
#include <vector>
#include <pthread.h>

class DlopenCb {
 private:
  static bool is_debug;
  std::set<std::string> hooked_libs;
  std::set<void (*)(std::set<std::string> &, int, std::string &)> callbacks;
  pthread_mutex_t add_lib_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t callback_mutex = PTHREAD_MUTEX_INITIALIZER;
  DlopenCb();
  ~DlopenCb() {};
  void Refresh(int source, std::string &loadLibName);
  void OnDlopen(const char *filename, int source);
  static void *HookDlopenExt(const char *filename, int flags, const android_dlextinfo *info);
  static void *HookDlopen(const char *filename, int flag);
 public:
  const static int dlopen_source_refresh = 0;
  const static int dlopen_source_android = 1;
  const static int dlopen_source_origin = 2;
  const static int dlopen_source_get_libs = 3;
  static DlopenCb &GetInstance() {
    static DlopenCb instance;
    return instance;
  }
  static pthread_mutex_t hook_mutex;
  void AddCallback(void(*callback)(std::set<std::string> &, int, std::string &));
  void RemoveCallback(void(*callback)(std::set<std::string> &, int, std::string &));
  void GetLoadedLibs(std::set<std::string>& libs, bool refresh = false);
  static void SetDebug(bool debug);
};
