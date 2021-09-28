/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by shenvsv on 2021.
 *
 */

#pragma once
#include <cstddef>
#include <string>
#include <set>
#include <android/dlext.h>
#include <vector>
#include <pthread.h>

class __attribute__((visibility("default"))) DlopenCb {
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
  static const int dlopen_source_refresh = 0;
  static const int dlopen_source_android = 1;
  static const int dlopen_source_origin = 2;
  static const int dlopen_source_get_libs = 3;
  static DlopenCb &GetInstance();
  static pthread_mutex_t hook_mutex;
  void AddCallback(void(*callback)(std::set<std::string> &, int, std::string &));
  void RemoveCallback(void(*callback)(std::set<std::string> &, int, std::string &));
  void GetLoadedLibs(std::set<std::string>& libs, bool refresh = false);
  static void SetDebug(bool debug);
};
