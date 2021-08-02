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

#include "leak_monitor.h"
#include "utils/auto_time.h"
#include "utils/log_util.h"
#include "kwai_dlfcn.h"
#include <asm/mman.h>
#include <dlfcn.h>
#include <utils/hook_helper.h>
#include <pthread.h>
#include <utils/stack_trace.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <thread_local_manager.h>
#include <unistd.h>
#include <unwind.h>
#include <assert.h>
#include <functional>
#include <regex>

namespace kwai {
namespace leak_monitor {

#define CLEAR_MEMORY(ptr, size) \
  do {    \
    if (ptr) { \
      memset(ptr, 0, size); \
    } \
  } while(0)

#define WRAP(x) x##Monitor
#define PROXY(ret_type, function, ...) \
    static ALWAYS_INLINE ret_type WRAP(function)(__VA_ARGS__)

static const uint32_t kClockThresholdPerFrame = CLOCKS_PER_SEC / 10000 / kMaxBacktraceSize;
static const uint32_t kCheckedFrameThreshold = kMaxBacktraceSize / 3;

static bool use_fast_unwind = true;

// Define allocator proxies; aligned_alloc included in API 28 and valloc/pvalloc can ignore in LP64
// So we can't proxy aligned_alloc/valloc/pvalloc.
PROXY(void, Free, void *ptr) {
  free(ptr);
  if (ptr) {
    LeakMonitor::GetInstance().UnregisterAlloc(reinterpret_cast<uintptr_t>(ptr));
  }
}

PROXY(void *, Malloc, size_t size) {
  void *result = nullptr;
  {
    AutoTime auto_time("malloc");
    result = malloc(size);
  }
  {
    AutoTime auto_time("OnMonitor");
    LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result), size);
  }
  {
    AutoTime auto_time("Clear MEM");
    CLEAR_MEMORY(result, size);
  }
  return result;
}

PROXY(void *, Realloc, void *ptr, size_t size) {
  auto result = realloc(ptr, size);
  if (ptr != nullptr) {
    LeakMonitor::GetInstance().UnregisterAlloc(reinterpret_cast<uintptr_t>(ptr));
  }
  LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result), size);
  return result;
}

PROXY(void *, Calloc, size_t item_count, size_t item_size) {
  auto result = calloc(item_count, item_size);
  LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result),
                                         item_count * item_size);
  return result;
}

PROXY(void *, Memalign, size_t alignment, size_t byte_count) {
  auto result = memalign(alignment, byte_count);
  LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result), byte_count);
  CLEAR_MEMORY(result, byte_count);
  return result;
}

PROXY(int, Posix_memalign, void** memptr, size_t alignment, size_t size) {
  auto result = posix_memalign(memptr, alignment, size);
  LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(*memptr), size);
  CLEAR_MEMORY(*memptr, size);
  return result;
}


struct BacktraceInfo {
  uint32_t cout;
  clock_t start;
};

static _Unwind_Reason_Code CallBack(_Unwind_Context *context, void *info) {
  // NOTE: _Unwind_GetIP Remove thumb mode bit
  auto pc = _Unwind_GetIP(context);
  auto curr = &(reinterpret_cast<BacktraceInfo *>(info)->cout);

  if (!pc || (++(*curr)) > kMaxBacktraceSize) {
    return _URC_CONTINUE_UNWIND;
  }

  auto ptr = ThreadLocalManager::GetAllocThreadInfo();
  if (ptr) {
    ptr->backtrace[(*curr) - 1] = reinterpret_cast<uintptr_t>(pc);
    ptr->cursor = *curr;
  }

  if (*curr % kCheckedFrameThreshold == 0) {
    auto start = reinterpret_cast<BacktraceInfo *>(info)->start;
    if (clock() - start >= (kClockThresholdPerFrame * (*curr))) {
      return _URC_CONTINUE_UNWIND;
    }
  }

  return _URC_NO_REASON;
}

static ALWAYS_INLINE void UnwindBacktrace() {
  if (use_fast_unwind) {
    auto ptr = ThreadLocalManager::GetAllocThreadInfo();
    if (ptr) {
      uintptr_t buff[kMaxBacktraceSize]{};
      int cur_count = 0;
      size_t frame_count = StackTrace::FastUnwind(buff, kMaxBacktraceSize);
      for (int i = 0; i < frame_count; ++i) {
        uintptr_t pc = buff[i];
        ptr->backtrace[cur_count++] = reinterpret_cast<uintptr_t>(pc);
        ptr->cursor = cur_count;
      }
    }
  } else {
    BacktraceInfo info = {.cout = 0, .start = clock()};
    _Unwind_Backtrace(CallBack, &info);
  }
}

LeakMonitor &LeakMonitor::GetInstance() {
  static LeakMonitor leak_monitor;
  return leak_monitor;
}

void LeakMonitor::InstallMonitor(std::vector<std::string> *selected_list,
                                   std::vector<std::string> *ignore_list) {
  CHECK(!has_install_monitor_);

  // Reinstall can't hook again
  if (is_hooked_) {
    has_install_monitor_ = true;
    return;
  }

  std::vector<const std::string> register_pattern = {"^/data/.*\\.so$"};
  std::vector<const std::string> ignore_pattern = {
      ".*/libnative-oom.so$",
      ".*/libxhook_lib.so$"};

  if (ignore_list != nullptr) {
    for (std::string &item : *ignore_list) {
      ignore_pattern.push_back(".*/" + item + ".so$");
    }
  }
  if (selected_list != nullptr && !selected_list->empty()) {
    // only hook the so in selected list
    register_pattern.clear();
    for (std::string &item : *selected_list) {
      register_pattern.push_back("^/data/.*/" + item + ".so$");
    }
  }
  std::vector<std::pair<const std::string, void *const>> hook_entries = {
      std::make_pair("malloc", reinterpret_cast<void *>(WRAP(Malloc))),
      std::make_pair("realloc", reinterpret_cast<void *>(WRAP(Realloc))),
      std::make_pair("calloc", reinterpret_cast<void *>(WRAP(Calloc))),
      std::make_pair("memalign", reinterpret_cast<void *>(WRAP(Memalign))),
      std::make_pair("posix_memalign", reinterpret_cast<void *>(WRAP(Posix_memalign))),
      std::make_pair("free", reinterpret_cast<void *>(WRAP(Free)))};

  StackTrace::Init();
  if (HookHelper::HookMethods(register_pattern, ignore_pattern, hook_entries) &&
      HookHelper::SyncRefreshHook()) {
    has_install_monitor_ = true;
    is_hooked_ = true;
    return;
  }

  RLOGE("%s Fail", __FUNCTION__);
}

void LeakMonitor::UninstallMonitor() {
  CHECK(has_install_monitor_);
  has_install_monitor_ = false;
  live_alloc_records_.Clear();
}

int LeakMonitor::SyncRefresh() {
  CHECK(has_install_monitor_);
  return HookHelper::SyncRefreshHook() ? EXIT_SUCCESS : EXIT_FAILURE;
}

int LeakMonitor::AsyncRefresh() {
  CHECK(has_install_monitor_);
  return HookHelper::AsyncRefreshHook() ? EXIT_SUCCESS : EXIT_FAILURE;
}

void LeakMonitor::SetAllocThreshold(size_t threshold) {
  CHECK(has_install_monitor_);
  alloc_threshold_ = threshold;
}

std::vector<std::shared_ptr<AllocRecord>> LeakMonitor::GetLeakAllocs() {
  CHECK(has_install_monitor_);
  auto unreachable_allocs = CollectUnreachableMem();
  std::vector<std::shared_ptr<AllocRecord>> live_allocs;
  std::vector<std::shared_ptr<AllocRecord>> leak_allocs;

  // Collect live memory blocks
  auto collect_func =
      [&](std::shared_ptr<AllocRecord> &alloc_info) -> void { live_allocs.push_back(alloc_info); };
  live_alloc_records_.Dump(collect_func);

  DLOGI("live_buckets size %d unreachable size %d", live_allocs.size(), unreachable_allocs.size());
  auto is_leak =
      [&](decltype(unreachable_allocs)::value_type &unreachable, std::shared_ptr<AllocRecord> &live) ->
      bool {
    auto live_start = CONFUSE(live->address);
    auto live_end = live_start + live->size;
    auto unreachable_start = unreachable.first;
    auto unreachable_end = unreachable_start + unreachable.second;
    // TODO why
    return live_start == unreachable_start ||
        live_start >= unreachable_start && live_end <= unreachable_end;
  };
  // Check leak allocation (unreachable && not free)
  for (auto &live : live_allocs) {
    for (auto &unreachable : unreachable_allocs) {
      DLOGI("live alloc %p <> unreachable %p", CONFUSE(live->address), unreachable.first);
      if (is_leak(unreachable, live)) {
        leak_allocs.push_back(live);
        // Just remove leak allocation(never be free)
        UnregisterAlloc(live->address);
      }
    }
  }

  return leak_allocs;
}

uint64_t LeakMonitor::CurrentAllocIndex() {
  CHECK(has_install_monitor_);
  return alloc_index_.load(std::memory_order_relaxed);
}

ALWAYS_INLINE void LeakMonitor::RegisterAlloc(uintptr_t address, size_t size) {
  if (!address || !size) {
    return;
  }

  UnwindBacktrace();
  auto alloc_record = std::make_shared<AllocRecord>();
  alloc_record->address = CONFUSE(address);
  alloc_record->size = size;
  alloc_record->index = alloc_index_++;
  auto alloc_thread_info = ThreadLocalManager::GetAllocThreadInfo();
  assert(alloc_thread_info != nullptr);
  alloc_record->num_backtraces = alloc_thread_info->cursor;
  memcpy(alloc_record->thread_name, alloc_thread_info->thread_name, kMaxThreadNameLen);
  memcpy(alloc_record->backtrace, alloc_thread_info->backtrace,
         sizeof(uintptr_t) * alloc_thread_info->cursor);
  live_alloc_records_.Put(CONFUSE(address), std::move(alloc_record));
}

ALWAYS_INLINE void LeakMonitor::UnregisterAlloc(uintptr_t address) {
  live_alloc_records_.Erase(address);
}

ALWAYS_INLINE void LeakMonitor::OnMonitor(uintptr_t address, size_t size) {
  if (!has_install_monitor_ || !address ||
      size < alloc_threshold_.load(std::memory_order_relaxed)) {
    return;
  }

  DLOGI("%s address %p, size %d", __FUNCTION__, address, size);

  {
    if (ThreadLocalManager::InitOnce()) {
      RLOGE("%s %d InitOnce fail", __FUNCTION__, __LINE__);
      return;
    }

    RegisterAlloc(address, size);
  }
}

std::vector<std::pair<uintptr_t, size_t>> LeakMonitor::CollectUnreachableMem() {
  std::vector<std::pair<uintptr_t, size_t>> unreachable_mem;
  auto handler = kwai::linker::DlFcn::dlopen("libmemunreachable.so", RTLD_NOW);
  if (!handler) {
    RLOGE("dlopen libmemunreachable error: %s", dlerror());
    return std::move(unreachable_mem);
  }

  auto get_unreachable_memory =
      kwai::linker::DlFcn::dlsym(handler, "_ZN7android26GetUnreachableMemoryStringEbm");
  if (!get_unreachable_memory) {
    RLOGE("dlsym get_unreachable_memory error: %s", dlerror());
    return std::move(unreachable_mem);
  }

  // libmemunreachable NOT work in release apk
  if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
    RLOGE("Set process dumpable Fail");
    return std::move(unreachable_mem);
  }

  // Note: time consuming
  std::string unreachable_memory = reinterpret_cast<std::string (*)(bool, size_t)>
      (get_unreachable_memory)(false, 1024);

//  prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

  std::regex filter_regex("[0-9]+ bytes unreachable at [A-Za-z0-9]+");
  std::sregex_iterator unreachable_begin(unreachable_memory.begin(),
                                         unreachable_memory.end(),
                                         filter_regex);
  std::sregex_iterator unreachable_end;
  for (; unreachable_begin != unreachable_end; ++unreachable_begin) {
    std::string line = unreachable_begin->str();
    auto address = std::stoul(line.substr(line.find_last_of(' ') + 1, line.length() - line
        .find_last_of(' ') - 1), 0, 16);
    auto size = std::stoul(line.substr(0, line.find_first_of(' ')));
    unreachable_mem.push_back(std::pair<uintptr_t, size_t>(address, size));
  }
  return std::move(unreachable_mem);
}
} // namespace leak_monitor
} // namespace kwai