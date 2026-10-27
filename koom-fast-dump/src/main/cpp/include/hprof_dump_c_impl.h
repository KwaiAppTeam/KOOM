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

#ifndef KOOM_HPROF_DUMP_C_IMPL_H_
#define KOOM_HPROF_DUMP_C_IMPL_H_

#include "hprof_dump_impl.h"

#include <memory>

#include "defines.h"

namespace kwai {
namespace leak_monitor {

class ScopedSuspendAllC;
class ScopedGCCriticalSectionC;
class MutexLockC;

/**
 * HprofDumpImpl for Android 17 (API 37).
 *
 * ART's perfetto_hprof::ForkUnderLocks() holds these locks before forking.
 * ReaderWriterMutex shared-lock APIs are ALWAYS_INLINE and cannot be called
 * through libart's dynamic symbol interface.
 */
class HprofDumpCImpl : public HprofDumpImpl {
 public:
  static HprofDumpCImpl &GetInstance();

 public:
  HprofDumpCImpl();
  ~HprofDumpCImpl() override = default;

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

  // art::Locks::thread_list_lock_, jni_weak_globals_lock_ and jit_lock_.
  void **thread_list_lock_ptr_;
  void **jni_weak_globals_lock_ptr_;
  void **jit_lock_ptr_;
  void (*exclusive_lock_fnc_)(void *, void *self);
  void (*exclusive_unlock_fnc_)(void *, void *self);

  void (*dump_heap_func_)(const char *filename, int, bool);

 private:
  std::unique_ptr<ScopedSuspendAllC> ssa_instance_;
  std::unique_ptr<ScopedGCCriticalSectionC> sgc_instance_;

 private:
  friend class ScopedSuspendAllC;
  friend class ScopedGCCriticalSectionC;
  friend class MutexLockC;
};

}  // namespace leak_monitor
}  // namespace kwai

#endif  // KOOM_HPROF_DUMP_C_IMPL_H_
