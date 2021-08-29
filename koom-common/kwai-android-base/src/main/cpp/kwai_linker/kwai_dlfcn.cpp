/*
 * Copyright (c) 2020. Kwai, Inc. All rights reserved.
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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2020.
 *
 */

#include <dlfcn.h>
#include <fcntl.h>
#include <kwai_linker/elf_reader.h>
#include <kwai_linker/kwai_dlfcn.h>
#include <kwai_util/kwai_macros.h>
#include <link.h>
#include <log/kcheck.h>
#include <log/log.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include "map_util.hpp"

#define LOG_TAG "kwai_dlfcn"

namespace kwai {
namespace linker {

int dl_iterate_phdr_wrapper(int (*__callback)(struct dl_phdr_info *, size_t,
                                              void *),
                            void *__data) {
  if (dl_iterate_phdr) {
    return dl_iterate_phdr(__callback, __data);
  }
  return 0;
}

int DlFcn::android_api_;

void DlFcn::init_api() {
  android_api_ = android_get_device_api_level();
  ALOGV("android_api_ = %d", android_api_);
}

static pthread_once_t once_control = PTHREAD_ONCE_INIT;

// Used for DlFcn::dlopen above android M
static int dl_iterate_callback(dl_phdr_info *info, size_t size, void *data) {
  ALOGV("dl_iterate_callback %s %p", info->dlpi_name, info->dlpi_addr);
  auto target = reinterpret_cast<DlFcn::dl_iterate_data *>(data);
  if (info->dlpi_addr != 0 &&
      strstr(info->dlpi_name, target->info_.dlpi_name)) {
    target->info_.dlpi_name = info->dlpi_name;
    target->info_.dlpi_addr = info->dlpi_addr;
    target->info_.dlpi_phdr = info->dlpi_phdr;
    target->info_.dlpi_phnum = info->dlpi_phnum;

    // break iterate
    return 1;
  }
  // continue iterate
  return 0;
}

using __loader_dlopen_fn = void *(*)(const char *filename, int flag,
                                     void *address);

KWAI_EXPORT void *DlFcn::dlopen(const char *lib_name, int flags) {
  pthread_once(&once_control, init_api);
  if (android_api_ < __ANDROID_API_N__) {
    return ::dlopen(lib_name, flags);
  }
  if (android_api_ >= __ANDROID_API_O__) {
    void *handle = ::dlopen("libdl.so", RTLD_NOW);
    KCHECKP(handle)
    auto __loader_dlopen = reinterpret_cast<__loader_dlopen_fn>(
        ::dlsym(handle, "__loader_dlopen"));
    KCHECKP(__loader_dlopen)
    if (android_api_ < __ANDROID_API_Q__) {
      return __loader_dlopen(lib_name, flags, (void *)dlerror);
    } else {
      handle = __loader_dlopen(lib_name, flags, (void *)dlerror);
      if (handle == nullptr) {
        // Android Q added "runtime" namespace
        dl_iterate_data data{};
        data.info_.dlpi_name = lib_name;
        dl_iterate_phdr_wrapper(dl_iterate_callback, &data);
        KCHECKP(data.info_.dlpi_addr > 0)
        handle = __loader_dlopen(lib_name, flags, (void *)data.info_.dlpi_addr);
      }
      return handle;
    }
  }
  // __ANDROID_API_N__ && __ANDROID_API_N_MR1__
  auto *data = new dl_iterate_data();
  data->info_.dlpi_name = lib_name;
  dl_iterate_phdr_wrapper(dl_iterate_callback, data);

  return data;
}

KWAI_EXPORT void *DlFcn::dlsym(void *handle, const char *name) {
  KCHECKP(handle)
  auto is_android_N = []() -> bool {
    return android_api_ == __ANDROID_API_N__ ||
           android_api_ == __ANDROID_API_N_MR1__;
  };

  if (!is_android_N()) {
    return ::dlsym(handle, name);
  }

  // __ANDROID_API_N__ && __ANDROID_API_N_MR1__
  auto *data = (dl_iterate_data *)handle;
  if (!data->info_.dlpi_name || data->info_.dlpi_name[0] != '/') {
    return nullptr;
  }

  ElfReader elf_reader(std::make_shared<FileElfWrapper>(data->info_.dlpi_name));
  if (!elf_reader.Init()) {
    return nullptr;
  }

  return elf_reader.LookupSymbol(name, data->info_.dlpi_addr, is_android_N());
}

KWAI_EXPORT int DlFcn::dlclose(void *handle) {
  if (android_api_ != __ANDROID_API_N__ &&
      android_api_ != __ANDROID_API_N_MR1__) {
    return ::dlclose(handle);
  }
  // __ANDROID_API_N__ && __ANDROID_API_N_MR1__
  delete (dl_iterate_data *)handle;
  return 0;
}

KWAI_EXPORT void *DlFcn::dlopen_elf(const char *lib_name, int flags) {
  pthread_once(&once_control, init_api);
  ElfW(Addr) load_base;
  std::string so_full_name;
  bool ret =
      MapUtil::GetLoadInfo(lib_name, &load_base, so_full_name, android_api_);

  if (!ret || so_full_name.empty() || so_full_name[0] != '/') {
    return nullptr;
  }

  SoDlInfo *so_dl_info = new (std::nothrow) SoDlInfo;
  if (!so_dl_info) {
    ALOGE("no memory for %s", so_full_name.c_str());
    return nullptr;
  }

  so_dl_info->load_base = load_base;
  so_dl_info->full_name = so_full_name;
  return so_dl_info;
}

KWAI_EXPORT void *DlFcn::dlsym_elf(void *handle, const char *name) {
  KCHECKP(handle)
  auto *so_dl_info = reinterpret_cast<SoDlInfo *>(handle);
  ElfReader elf_reader(
      std::make_shared<FileElfWrapper>(so_dl_info->full_name.c_str()));

  if (!elf_reader.Init()) {
    return nullptr;
  }

  return elf_reader.LookupSymbol(name, so_dl_info->load_base);
}

KWAI_EXPORT int DlFcn::dlclose_elf(void *handle) {
  KCHECKI(handle)
  delete reinterpret_cast<SoDlInfo *>(handle);
  return 0;
}

}  // namespace linker

}  // namespace kwai