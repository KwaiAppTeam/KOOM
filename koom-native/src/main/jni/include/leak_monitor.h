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

#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_LEAK_MONITOR_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_LEAK_MONITOR_H_

#include <linux/prctl.h>
#include <sys/prctl.h>

#include <list>
#include <vector>

#include "constants.h"
#include "memory_analyzer.h"
#include "utils/concurrent_hash_map.h"

#define CONFUSE(address) (~(address))

namespace kwai {
namespace leak_monitor {
struct AllocRecord {
  uint64_t index;
  uint32_t size;
  intptr_t address;
  uint32_t num_backtraces;
  uintptr_t backtrace[kMaxBacktraceSize];
  char thread_name[kMaxThreadNameLen];
};

struct ThreadInfo {
  char name[kMaxThreadNameLen];
  ThreadInfo() {
    if (prctl(PR_GET_NAME, name)) {
      memcpy(name, "unknown", kMaxThreadNameLen);
    }
  }

  ~ThreadInfo() = default;
};

class LeakMonitor {
 public:
  static LeakMonitor &GetInstance();
  bool Install(std::vector<std::string> *selected_list,
               std::vector<std::string> *ignore_list);
  void Uninstall();
  void SetMonitorThreshold(size_t threshold);
  std::vector<std::shared_ptr<AllocRecord>> GetLeakAllocs();
  uint64_t CurrentAllocIndex();
  void OnMonitor(uintptr_t address, size_t size);
  void RegisterAlloc(uintptr_t address, size_t size);
  void UnregisterAlloc(uintptr_t address);

 private:
  LeakMonitor()
      : alloc_index_(0),
        has_install_monitor_(false),
        live_alloc_records_(),
        alloc_threshold_(kDefaultAllocThreshold),
        memory_analyzer_() {}
  ~LeakMonitor() = default;
  LeakMonitor(const LeakMonitor &);
  LeakMonitor &operator=(const LeakMonitor &);
  std::unique_ptr<MemoryAnalyzer> memory_analyzer_;
  ConcurrentHashMap<intptr_t, std::shared_ptr<AllocRecord>> live_alloc_records_;
  std::atomic<uint64_t> alloc_index_;
  std::atomic<bool> has_install_monitor_;
  std::atomic<size_t> alloc_threshold_;
};
}  // namespace leak_monitor
}  // namespace kwai
#endif  // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_LEAK_MONITOR_H_
