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
 * Created by lbtrace on 2021.
 *
 */

#define LOG_TAG "memory_analyzer"
#include "memory_analyzer.h"

#include <dlfcn.h>
#include <log/log.h>
#include <sys/prctl.h>

#include <regex>

#include "kwai_linker/kwai_dlfcn.h"

namespace kwai {
namespace leak_monitor {
static const char *kLibMemUnreachableName = "libmemunreachable.so";
// Just need the symbol in arm64-v8a so
// API level > Android O
static const char *kGetUnreachableMemoryStringSymbolAboveO =
    "_ZN7android26GetUnreachableMemoryStringEbm";
// API level <= Android O
static const char *kGetUnreachableMemoryStringSymbolBelowO =
    "_Z26GetUnreachableMemoryStringbm";

MemoryAnalyzer::MemoryAnalyzer()
    : get_unreachable_fn_(nullptr), handle_(nullptr) {
  auto handle = kwai::linker::DlFcn::dlopen(kLibMemUnreachableName, RTLD_NOW);
  if (!handle) {
    ALOGE("dlopen %s error: %s", kLibMemUnreachableName, dlerror());
    return;
  }

  if (android_get_device_api_level() > __ANDROID_API_O__) {
    get_unreachable_fn_ =
        reinterpret_cast<GetUnreachableFn>(kwai::linker::DlFcn::dlsym(
            handle, kGetUnreachableMemoryStringSymbolAboveO));
  } else {
    get_unreachable_fn_ =
        reinterpret_cast<GetUnreachableFn>(kwai::linker::DlFcn::dlsym(
            handle, kGetUnreachableMemoryStringSymbolBelowO));
  }
}

MemoryAnalyzer::~MemoryAnalyzer() {
  if (handle_) {
    kwai::linker::DlFcn::dlclose(handle_);
  }
}

bool MemoryAnalyzer::IsValid() { return get_unreachable_fn_ != nullptr; }

std::vector<std::pair<uintptr_t, size_t>>
MemoryAnalyzer::CollectUnreachableMem() {
  std::vector<std::pair<uintptr_t, size_t>> unreachable_mem;

  if (!IsValid()) {
    ALOGE("MemoryAnalyzer NOT valid");
    return std::move(unreachable_mem);
  }

  // libmemunreachable NOT work in release apk because it using ptrace
  if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
    ALOGE("Set process dumpable Fail");
    return std::move(unreachable_mem);
  }

  // Note: time consuming
  std::string unreachable_memory = get_unreachable_fn_(false, 1024);

  // Unset "dumpable" for security
  prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

  std::regex filter_regex("[0-9]+ bytes unreachable at [A-Za-z0-9]+");
  std::sregex_iterator unreachable_begin(
      unreachable_memory.begin(), unreachable_memory.end(), filter_regex);
  std::sregex_iterator unreachable_end;
  for (; unreachable_begin != unreachable_end; ++unreachable_begin) {
    const auto& line = unreachable_begin->str();
    auto address =
        std::stoul(line.substr(line.find_last_of(' ') + 1,
                               line.length() - line.find_last_of(' ') - 1),
                   0, 16);
    auto size = std::stoul(line.substr(0, line.find_first_of(' ')));
    unreachable_mem.emplace_back(address, size);
  }
  return std::move(unreachable_mem);
}
}  // namespace leak_monitor
}  // namespace kwai