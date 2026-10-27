/*
 * Copyright (c) 2025. Kwai, Inc. All rights reserved.
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
 */

#ifndef KOOM_HPROF_DUMP_B_IMPL_H_
#define KOOM_HPROF_DUMP_B_IMPL_H_

#include "hprof_dump_impl.h"

#include <memory>

#include "defines.h"

namespace kwai {
namespace leak_monitor {

class ScopedSuspendAllB;
class ScopedGCCriticalSectionB;
class MutexLockB;

/**
 * HprofDumpImpl for Android 16 (API 36).
 */
class HprofDumpBImpl : public HprofDumpImpl {
 public:
  static HprofDumpBImpl &GetInstance();

 public:
  HprofDumpBImpl();
  ~HprofDumpBImpl() override = default;

 public:
  bool Initialize() override;

 public:
  bool Suspend() override;
  pid_t Fork() override;
  bool Resume() override;

  void DumpHeap(const char *filename) override;

 private:
  bool init_done_;

 private:
  void (*ssa_constructor_fnc_)(void *handle, const char *cause, bool long_suspend);
  void (*ssa_destructor_fnc_)(void *handle);
  void (*sgc_constructor_fnc_)(void *handle, void *self, GcCause cause,
                               CollectorType collector_type);
  void (*sgc_destructor_fnc_)(void *handle);

  // art::Locks::thread_list_lock_
  void **thread_list_lock_ptr_;
  void (*exclusive_lock_fnc_)(void *, void *self);
  void (*exclusive_unlock_fnc_)(void *, void *self);

  void (*dump_heap_func_)(const char *filename, int, bool);

 private:
  std::unique_ptr<ScopedSuspendAllB> ssa_instance_;
  std::unique_ptr<ScopedGCCriticalSectionB> sgc_instance_;

 private:
  friend class ScopedSuspendAllB;
  friend class ScopedGCCriticalSectionB;
  friend class MutexLockB;
};

}  // namespace leak_monitor
}  // namespace kwai

#endif  // KOOM_HPROF_DUMP_B_IMPL_H_
