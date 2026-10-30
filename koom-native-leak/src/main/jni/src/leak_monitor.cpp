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

#define LOG_TAG "leak_monitor"
#include "leak_monitor.h"

#include <asm/mman.h>
#include <assert.h>
#include <dlfcn.h>
#include <kwai_util/kwai_macros.h>
#include <log/kcheck.h>
#include <log/log.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <unwind.h>
#include <utils/hook_helper.h>
#include <utils/stack_trace.h>

#include <functional>
#include <regex>
#include <thread>

#include "kwai_linker/kwai_dlfcn.h"
#include "utils/auto_time.h"

namespace kwai {
namespace memory_monitor {

#define WRAP(x) x##Monitor
#define HOOK(ret_type, function, ...) \
  static ALWAYS_INLINE ret_type WRAP(function)(__VA_ARGS__)

// Define allocator proxies; aligned_alloc included in API 28 and valloc/pvalloc
// can ignore in LP64 So we can't proxy aligned_alloc/valloc/pvalloc.
HOOK(void, free, void *ptr) {
  free(ptr);
  if (ptr) {
    LeakMonitor::GetInstance().UnregisterAlloc(
        reinterpret_cast<uintptr_t>(ptr));
  }
}

HOOK(void *, malloc, size_t size) {
  auto result = malloc(size);
  LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result),
                                       size);
  return result;
}

HOOK(void *, realloc, void *ptr, size_t size) {
  auto result = realloc(ptr, size);
  // A failed realloc(size > 0) leaves the original allocation alive. Keep its
  // record until it is actually freed or successfully reallocated.
  if (ptr != nullptr && (result != nullptr || size == 0)) {
    LeakMonitor::GetInstance().UnregisterAlloc(
        reinterpret_cast<uintptr_t>(ptr));
  }
  if (result != nullptr) {
    LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result),
                                         size);
  }
  return result;
}

HOOK(void *, calloc, size_t item_count, size_t item_size) {
  auto result = calloc(item_count, item_size);
  if (result != nullptr) {
    LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result),
                                         item_count * item_size);
  }
  return result;
}

HOOK(void *, memalign, size_t alignment, size_t byte_count) {
  auto result = memalign(alignment, byte_count);
  LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(result),
                                       byte_count);
  return result;
}

HOOK(int, posix_memalign, void **memptr, size_t alignment, size_t size) {
  auto result = posix_memalign(memptr, alignment, size);
  if (result == 0 && memptr != nullptr && *memptr != nullptr) {
    LeakMonitor::GetInstance().OnMonitor(reinterpret_cast<intptr_t>(*memptr),
                                         size);
  }
  return result;
}

LeakMonitor &LeakMonitor::GetInstance() {
  static LeakMonitor leak_monitor;
  return leak_monitor;
}

bool LeakMonitor::Install(std::vector<std::string> *selected_list,
                          std::vector<std::string> *ignore_list) {
  KCHECK(!has_install_monitor_);

  // Reinstall can't hook again
  if (has_install_monitor_) {
    return true;
  }

  live_alloc_records_.Clear();
  alloc_index_.store(0, std::memory_order_relaxed);
  alloc_threshold_.store(kDefaultAllocThreshold, std::memory_order_relaxed);
  intercept_mode_.store(InterceptMode::FULL, std::memory_order_relaxed);
  ever_sampled_.store(false, std::memory_order_relaxed);
  memory_analyzer_ = std::make_unique<MemoryAnalyzer>();
  if (!memory_analyzer_->IsValid()) {
    ALOGE("memory_analyzer_ NOT Valid");
    return false;
  }

  std::vector<const std::string> register_pattern = {"^/data/.*\\.so$"};
  std::vector<const std::string> ignore_pattern = {".*/libkoom-native.so$",
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
      std::make_pair("malloc", reinterpret_cast<void *>(WRAP(malloc))),
      std::make_pair("realloc", reinterpret_cast<void *>(WRAP(realloc))),
      std::make_pair("calloc", reinterpret_cast<void *>(WRAP(calloc))),
      std::make_pair("memalign", reinterpret_cast<void *>(WRAP(memalign))),
      std::make_pair("posix_memalign",
                     reinterpret_cast<void *>(WRAP(posix_memalign))),
      std::make_pair("free", reinterpret_cast<void *>(WRAP(free)))};

  if (HookHelper::HookMethods(register_pattern, ignore_pattern, hook_entries)) {
    has_install_monitor_ = true;
    return true;
  }

  HookHelper::UnHookMethods();
  live_alloc_records_.Clear();
  memory_analyzer_.reset(nullptr);
  ALOGE("%s Fail", __FUNCTION__);
  return false;
}

void LeakMonitor::Uninstall() {
  KCHECKV(has_install_monitor_)
  has_install_monitor_ = false;
  intercept_mode_.store(InterceptMode::STOPPED, std::memory_order_relaxed);
  HookHelper::UnHookMethods();
  live_alloc_records_.Clear();
  memory_analyzer_.reset(nullptr);
}

void LeakMonitor::SetMonitorThreshold(size_t threshold) {
  KCHECK(has_install_monitor_);
  if (threshold < kDefaultAllocThreshold) {
    ALOGW("Intercept threshold must be over %d but your value is %zu",
          kDefaultAllocThreshold, threshold);
    threshold = kDefaultAllocThreshold;
  }
  alloc_threshold_ = threshold;
}

void LeakMonitor::SetSamplingWatermark(uint32_t high_watermark, uint32_t low_watermark) {
  if (high_watermark > 0) {
    sampling_high_watermark_.store(high_watermark, std::memory_order_relaxed);
  }
  if (low_watermark > 0) {
    sampling_low_watermark_.store(low_watermark, std::memory_order_relaxed);
  }
}

std::vector<std::shared_ptr<AllocRecord>> LeakMonitor::GetLeakAllocs() {
  KCHECK(has_install_monitor_);
  auto unreachable_allocs = memory_analyzer_->CollectUnreachableMem();
  std::vector<std::shared_ptr<AllocRecord>> live_allocs;
  std::vector<std::shared_ptr<AllocRecord>> leak_allocs;

  // Collect live memory blocks
  auto collect_func = [&](std::shared_ptr<AllocRecord> &alloc_info) -> void {
    live_allocs.push_back(alloc_info);
  };
  live_alloc_records_.Dump(collect_func);

  auto is_leak = [&](decltype(unreachable_allocs)::value_type &unreachable,
                     std::shared_ptr<AllocRecord> &live) -> bool {
    // Android may tag the top byte of allocated pointers.
    constexpr uintptr_t kAddrMask = 0x00FFFFFFFFFFFFFFULL;
    auto live_start = CONFUSE(live->address) & kAddrMask;
    auto live_end = live_start + live->size;
    auto unreachable_start = unreachable.first & kAddrMask;
    auto unreachable_end = unreachable_start + unreachable.second;
    return live_start == unreachable_start ||
           live_start >= unreachable_start && live_end <= unreachable_end;
  };
  // Check leak allocation (unreachable && not free)
  for (auto &live : live_allocs) {
    for (auto &unreachable : unreachable_allocs) {
      if (is_leak(unreachable, live)) {
        leak_allocs.push_back(live);
        // Just remove leak allocation(never be free)
        // live->address has been confused, we need to revert it first
        UnregisterAlloc(CONFUSE(live->address));
        break;
      }
    }
  }

  if (intercept_mode_.load(std::memory_order_relaxed) == InterceptMode::SAMPLING) {
    auto current_size = live_alloc_records_.Size();
    auto low_watermark = sampling_low_watermark_.load(std::memory_order_relaxed);
    if (current_size < low_watermark) {
      auto expected = InterceptMode::SAMPLING;
      if (intercept_mode_.compare_exchange_strong(expected, InterceptMode::FULL,
                                                  std::memory_order_relaxed)) {
        ALOGI("Intercept mode SAMPLING -> FULL (map size=%zu, low=%u)", current_size, low_watermark);
      }
    }
  }

  ALOGI("Leak analysis: live_allocs=%zu, unreachable=%zu, leaks_found=%zu, "
        "map_size=%zu",
        live_allocs.size(), unreachable_allocs.size(), leak_allocs.size(),
        LiveAllocCount());

  return leak_allocs;
}

uint64_t LeakMonitor::CurrentAllocIndex() {
  KCHECK(has_install_monitor_);
  return alloc_index_.load(std::memory_order_relaxed);
}

uint8_t LeakMonitor::CurrentInterceptMode() {
  return static_cast<uint8_t>(intercept_mode_.load(std::memory_order_relaxed));
}

bool LeakMonitor::EverSampled() {
  return ever_sampled_.load(std::memory_order_relaxed);
}

size_t LeakMonitor::LiveAllocCount() {
  return live_alloc_records_.Size();
}

ALWAYS_INLINE void LeakMonitor::RegisterAlloc(uintptr_t address, size_t size) {
  if (!address || !size) {
    return;
  }

  auto mode = intercept_mode_.load(std::memory_order_relaxed);
  if (mode == InterceptMode::STOPPED) {
    return;
  }
  // TODO: Small allocations skipped by deterministic sampling have no current
  // compensation path. Counts and bytes estimated from sample_weight can have
  // boundary error; possible follow-ups include deferred compensation or
  // aggregation by repeated backtraces.
  bool is_large = size >= kLargeAllocThreshold;
  if (mode == InterceptMode::SAMPLING && !is_large) {
    thread_local uint32_t sample_counter = 0;
    if (++sample_counter < kSmallAllocSamplingRate) {
      return;
    }
    sample_counter = 0;
  }

  thread_local ThreadInfo thread_info;
  auto alloc_record = std::make_shared<AllocRecord>();
  alloc_record->address = CONFUSE(address);
  alloc_record->size = size;
  alloc_record->sample_weight =
      (mode == InterceptMode::SAMPLING && !is_large) ? kSmallAllocSamplingRate
                                                     : 1;
  alloc_record->index = alloc_index_++;
  alloc_record->thread_name = thread_info.name;
  StackTrace::FastUnwind(alloc_record->backtrace, kMaxBacktraceSize);
  if (alloc_record->index % 1000 == 0) {
    auto current_size = live_alloc_records_.Size();
    auto high_watermark = sampling_high_watermark_.load(std::memory_order_relaxed);
    if (intercept_mode_.load(std::memory_order_relaxed) == InterceptMode::FULL &&
        current_size > high_watermark) {
      auto expected = InterceptMode::FULL;
      if (intercept_mode_.compare_exchange_strong(expected, InterceptMode::SAMPLING,
                                                  std::memory_order_relaxed)) {
        ever_sampled_.store(true, std::memory_order_relaxed);
        ALOGI("Intercept mode FULL -> SAMPLING (map size=%zu, high=%u)", current_size, high_watermark);
      }
    }
  }
  live_alloc_records_.Put(CONFUSE(address), std::move(alloc_record));
}

ALWAYS_INLINE void LeakMonitor::UnregisterAlloc(uintptr_t address) {
  // XHook cannot restore already-patched PLT entries, so calls can still reach
  // this proxy after uninstall. Match the internal unhooked behavior by making
  // those stale calls no-ops.
  if (!has_install_monitor_.load(std::memory_order_relaxed)) {
    return;
  }
  live_alloc_records_.Erase(CONFUSE(address));
}

ALWAYS_INLINE void LeakMonitor::OnMonitor(uintptr_t address, size_t size) {
  if (!has_install_monitor_ || !address ||
      size < alloc_threshold_.load(std::memory_order_relaxed)) {
    return;
  }

  RegisterAlloc(address, size);
}
}  // namespace memory_monitor
}  // namespace kwai
