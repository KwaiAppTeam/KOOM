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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2021.
 *
 */

#ifndef KOOM_HPROF_DUMP_H
#define KOOM_HPROF_DUMP_H

#include <android-base/macros.h>

#include <memory>
#include <string>

namespace kwai {
namespace leak_monitor {

// What caused the GC?
enum GcCause {
  // Invalid GC cause used as a placeholder.
  kGcCauseNone,
  // GC triggered by a failed allocation. Thread doing allocation is blocked
  // waiting for GC before
  // retrying allocation.
  kGcCauseForAlloc,
  // A background GC trying to ensure there is free memory ahead of allocations.
  kGcCauseBackground,
  // An explicit System.gc() call.
  kGcCauseExplicit,
  // GC triggered for a native allocation when NativeAllocationGcWatermark is
  // exceeded.
  // (This may be a blocking GC depending on whether we run a non-concurrent
  // collector).
  kGcCauseForNativeAlloc,
  // GC triggered for a collector transition.
  kGcCauseCollectorTransition,
  // Not a real GC cause, used when we disable moving GC (currently for
  // GetPrimitiveArrayCritical).
  kGcCauseDisableMovingGc,
  // Not a real GC cause, used when we trim the heap.
  kGcCauseTrim,
  // Not a real GC cause, used to implement exclusion between GC and
  // instrumentation.
  kGcCauseInstrumentation,
  // Not a real GC cause, used to add or remove app image spaces.
  kGcCauseAddRemoveAppImageSpace,
  // Not a real GC cause, used to implement exclusion between GC and debugger.
  kGcCauseDebugger,
  // GC triggered for background transition when both foreground and background
  // collector are CMS.
  kGcCauseHomogeneousSpaceCompact,
  // Class linker cause, used to guard filling art methods with special values.
  kGcCauseClassLinker,
  // Not a real GC cause, used to implement exclusion between code cache
  // metadata and GC.
  kGcCauseJitCodeCache,
  // Not a real GC cause, used to add or remove system-weak holders.
  kGcCauseAddRemoveSystemWeakHolder,
  // Not a real GC cause, used to prevent hprof running in the middle of GC.
  kGcCauseHprof,
  // Not a real GC cause, used to prevent GetObjectsAllocated running in the
  // middle of GC.
  kGcCauseGetObjectsAllocated,
  // GC cause for the profile saver.
  kGcCauseProfileSaver,
  // GC cause for running an empty checkpoint.
  kGcCauseRunEmptyCheckpoint,
};

// Which types of collections are able to be performed.
enum CollectorType {
  // No collector selected.
  kCollectorTypeNone,
  // Non concurrent mark-sweep.
  kCollectorTypeMS,
  // Concurrent mark-sweep.
  kCollectorTypeCMS,
  // Semi-space / mark-sweep hybrid, enables compaction.
  kCollectorTypeSS,
  // Heap trimming collector, doesn't do any actual collecting.
  kCollectorTypeHeapTrim,
  // A (mostly) concurrent copying collector.
  kCollectorTypeCC,
  // The background compaction of the concurrent copying collector.
  kCollectorTypeCCBackground,
  // Instrumentation critical section fake collector.
  kCollectorTypeInstrumentation,
  // Fake collector for adding or removing application image spaces.
  kCollectorTypeAddRemoveAppImageSpace,
  // Fake collector used to implement exclusion between GC and debugger.
  kCollectorTypeDebugger,
  // A homogeneous space compaction collector used in background transition
  // when both foreground and background collector are CMS.
  kCollectorTypeHomogeneousSpaceCompact,
  // Class linker fake collector.
  kCollectorTypeClassLinker,
  // JIT Code cache fake collector.
  kCollectorTypeJitCodeCache,
  // Hprof fake collector.
  kCollectorTypeHprof,
  // Fake collector for installing/removing a system-weak holder.
  kCollectorTypeAddRemoveSystemWeakHolder,
  // Fake collector type for GetObjectsAllocated
  kCollectorTypeGetObjectsAllocated,
  // Fake collector type for ScopedGCCriticalSection
  kCollectorTypeCriticalSection,
};

class HprofDump {
 public:
  static HprofDump &GetInstance();
  void Initialize();
  pid_t SuspendAndFork();
  bool ResumeAndWait(pid_t pid);

 private:
  HprofDump();
  ~HprofDump() = default;
  DISALLOW_COPY_AND_ASSIGN(HprofDump);

  bool init_done_;
  int android_api_;

  // ScopedSuspendAll instance placeholder
  std::unique_ptr<char[]> ssa_instance_;
  // ScopedGCCriticalSection instance placeholder
  std::unique_ptr<char[]> sgc_instance_;

  /**
   * Function pointer for ART <= Android Q
   */
  // art::Dbg::SuspendVM
  void (*suspend_vm_fnc_)();
  // art::Dbg::ResumeVM
  void (*resume_vm_fnc_)();

  /**
   * Function pointer for ART Android R
   */
  // art::ScopedSuspendAll::ScopedSuspendAll()
  void (*ssa_constructor_fnc_)(void *handle, const char *cause,
                               bool long_suspend);
  // art::ScopedSuspendAll::~ScopedSuspendAll()
  void (*ssa_destructor_fnc_)(void *handle);
  // art::gc::ScopedGCCriticalSection::ScopedGCCriticalSection()
  void (*sgc_constructor_fnc_)(void *handle, void *self, GcCause cause,
                               CollectorType collector_type);
  // art::gc::ScopedGCCriticalSection::~ScopedGCCriticalSection()
  void (*sgc_destructor_fnc_)(void *handle);
  // art::Locks::mutator_lock_
  void **mutator_lock_ptr_;
  // art::ReaderWriterMutex::ExclusiveLock
  void (*exclusive_lock_fnc_)(void *, void *self);
  // art::ReaderWriterMutex::ExclusiveUnlock
  void (*exclusive_unlock_fnc_)(void *, void *self);
};

}  // namespace leak_monitor
}  // namespace kwai
#endif  // KOOM_HPROF_DUMP_H
