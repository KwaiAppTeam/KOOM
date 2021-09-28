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

#define LOG_TAG "dlopencb"

#include <link.h>
#include <utility>
#include "dlopencb.h"
#include "xhook.h"
#include "xh_log.h"


// 兼容编译失败，实际API 21以下不支持开启
#if __ANDROID_API__ < 21
void* android_dlopen_ext(const char* __filename, int __flags, const android_dlextinfo* __info) {
    return 0;
}
int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info*, size_t, void*), void* __data) {
  return 0;
}
#endif

const char *dlopen_ignore_libs[] = {
    "kwai-android-base", "qti_performance",
    "xhook", "kxqpplatform.so",
    "/product/lib", "/vendor/lib"};

DlopenCb::DlopenCb() {
  std::string empty;
  Refresh(dlopen_source_refresh, empty);
}

bool DlopenCb::is_debug = false;
pthread_mutex_t DlopenCb::hook_mutex =
    PTHREAD_MUTEX_INITIALIZER;

static bool hookDlopen(const std::string &lib) {
  if (lib.find(".so") != std::string::npos) {
    for (const auto &ignoreLib : dlopen_ignore_libs) {
      if (lib.find(ignoreLib) != std::string::npos) {
        return false;
      }
    }
    return true;
  }
  return false;
}

int Callback(struct dl_phdr_info *info, size_t size, void *data) {
  auto *pair = static_cast<std::pair<std::set<std::string> *, std::set<std::string> *> *>(data);
  auto origin = pair->first;
  auto add = pair->second;
  auto name = info->dlpi_name;
  if (name != nullptr && hookDlopen(name) && origin->insert(name).second) {
    add->insert(name);
  }
  return 0;
}

void DlopenCb::Refresh(int source, std::string &loadLibName) {
  XH_LOG_INFO("Refresh start %d", source);
  std::set<std::string> addLibs;
  pthread_mutex_lock(&add_lib_mutex);
  auto callbackData =
      make_pair(&hooked_libs, &addLibs);
  dl_iterate_phdr(Callback, &callbackData);
  pthread_mutex_unlock(&add_lib_mutex);

  if (!addLibs.empty()) {
    pthread_mutex_lock(&hook_mutex);
    xhook_clear();
    if (is_debug) {
      xhook_enable_sigsegv_protection(0);
      xhook_enable_debug(1);
    } else {
      xhook_enable_sigsegv_protection(1);
    }
    for (const auto &lib : addLibs) {
      auto lib_ctr = lib.c_str();
      xhook_register(lib_ctr, "android_dlopen_ext", (void *) (HookDlopenExt), nullptr);
//      xhook_register(lib_ctr, "dlopen", (void *) (HookDlopen), nullptr);
      XH_LOG_INFO("Refresh new lib added %s", lib_ctr);
    }
    xhook_refresh(0);
    pthread_mutex_unlock(&hook_mutex);

    // notify
    XH_LOG_INFO("Refresh hooked");
    pthread_mutex_lock(&callback_mutex);
    for (auto &callback:callbacks) {
      callback(addLibs, source, loadLibName);
    }
    pthread_mutex_unlock(&callback_mutex);
  } else {
    XH_LOG_INFO("Refresh no lib found");
  }
}

void *DlopenCb::HookDlopenExt(const char *filename, int flags, const android_dlextinfo *info) {
  void *result = android_dlopen_ext(filename, flags, info);
  if (result != nullptr) {
    GetInstance().OnDlopen(filename, dlopen_source_android);
  }
  return result;
}

void *DlopenCb::HookDlopen(const char *filename, int flag) {
//  void *result = kwai::linker::DlFcn::dlopen(filename, flag);
//  if (result != nullptr) {
//    GetInstance().OnDlopen(filename, dlopen_source_origin);
//  }
//  return result;
  return nullptr;
}

void DlopenCb::OnDlopen(const char *filename, int source) {
  if (filename == nullptr || strlen(filename) == 0) {
    return;
  }
  XH_LOG_INFO("OnDlopen %d, %s", source, filename);
  auto name = std::string(filename);
  GetInstance().Refresh(source, name);
}

void DlopenCb::AddCallback(void (*callback)(std::set<std::string> &, int, std::string &)) {
  XH_LOG_INFO("AddCallback %p", callback);
  pthread_mutex_lock(&callback_mutex);
  callbacks.insert(callback);
  pthread_mutex_unlock(&callback_mutex);
}

void DlopenCb::RemoveCallback(void (*callback)(std::set<std::string> &, int, std::string &)) {
  XH_LOG_INFO("RemoveCallback %p", callback);
  pthread_mutex_lock(&callback_mutex);
  callbacks.erase(callback);
  pthread_mutex_unlock(&callback_mutex);
}

void DlopenCb::SetDebug(bool debug) {
  is_debug = debug;
}

void DlopenCb::GetLoadedLibs(std::set<std::string> &libs, bool refresh) {
  if (refresh) {
    std::string empty;
    Refresh(dlopen_source_get_libs, empty);
  }
  XH_LOG_INFO("GetLoadedLibs origin %d", hooked_libs.size());
  pthread_mutex_lock(&add_lib_mutex);
  std::copy(
      hooked_libs.begin(), hooked_libs.end(),
      std::inserter(libs, libs.begin()));
  pthread_mutex_unlock(&add_lib_mutex);
}

DlopenCb &DlopenCb::GetInstance() {
  static DlopenCb instance;
  return instance;
}

