//
// Created by zack on 2025/1/26.
//

#ifndef KOOM_HPROF_DUMP_BELOW_V_IMPL_H_
#define KOOM_HPROF_DUMP_BELOW_V_IMPL_H_

#include "hprof_dump_impl.h"

#include <memory>

#include "defines.h"

namespace kwai {
namespace leak_monitor {

/**
 * HprofDumpImpl for Android 11 - 14
 *
 * use art::ScopedSuspendAll::ScopedSuspendAll to Suspend & Resume,
 * use art::hprof::DumpHeap to DumpHeap
 */
class HprofDumpBelowVImpl : public HprofDumpImpl {
 public:
  static HprofDumpBelowVImpl &GetInstance();

 public:
  HprofDumpBelowVImpl();
  ~HprofDumpBelowVImpl() override = default;

 public:
  bool Initialize() override;

 public:
  bool Suspend() override;
  bool Resume() override;

  void DumpHeap(const char* filename) override;

 private:
  bool init_done_;

  // art::ScopedSuspendAll::ScopedSuspendAll()
  void (*ssa_constructor_fnc_)(void *handle, const char *cause, bool long_suspend);
  // art::ScopedSuspendAll::~ScopedSuspendAll()
  void (*ssa_destructor_fnc_)(void *handle);
  // art::gc::ScopedGCCriticalSection::ScopedGCCriticalSection()
  void (*sgc_constructor_fnc_)(void *handle, void *self, GcCause cause, CollectorType collector_type);
  // art::gc::ScopedGCCriticalSection::~ScopedGCCriticalSection()
  void (*sgc_destructor_fnc_)(void *handle);
  // art::Locks::mutator_lock_
  void **mutator_lock_ptr_;
  // art::ReaderWriterMutex::ExclusiveLock
  void (*exclusive_lock_fnc_)(void *, void *self);
  // art::ReaderWriterMutex::ExclusiveUnlock
  void (*exclusive_unlock_fnc_)(void *, void *self);

  // art::hprof::DumpHeap()
  void (*dump_heap_func_)(const char *filename, int, bool);

  // class ScopedSuspendAll : public ValueObject {
  // };
  //
  // class ValueObject {
  // };
  //
  // ScopedSuspendAll instance placeholder
  std::unique_ptr<char[]> ssa_instance_;
  // class GCCriticalSection {
  //  private:
  //   Thread* const self_;
  //   const char* section_name_;
  // };
  //
  // class ScopedGCCriticalSection {
  //  private:
  //   GCCriticalSection critical_section_;
  //   const char* old_no_suspend_reason_;
  // };
  //
  // ScopedGCCriticalSection instance placeholder
  std::unique_ptr<char[]> sgc_instance_;
};

} // namespace leak_monitor
} // namespace kwai

#endif //KOOM_HPROF_DUMP_BELOW_V_IMPL_H_
