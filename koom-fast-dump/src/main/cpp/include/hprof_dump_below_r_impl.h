//
// Created by zack on 2025/1/26.
//

#ifndef KOOM_HPROF_DUMP_BELOW_R_IMPL_H_
#define KOOM_HPROF_DUMP_BELOW_R_IMPL_H_

#include "hprof_dump_impl.h"

namespace kwai {
namespace leak_monitor {

/**
 * HprofDumpImpl for Android 5 - 10
 *
 * use art::Dbg::SuspendVM to Suspend,
 * use art::Dbg::ResumeVM to Resume,
 * use art::hprof::DumpHeap to DumpHeap
 */
class HprofDumpBelowRImpl : public HprofDumpImpl {
 public:
  static HprofDumpBelowRImpl &GetInstance();

 public:
  HprofDumpBelowRImpl();
  ~HprofDumpBelowRImpl() override = default;

 public:
  bool Initialize() override;

 public:
  bool Suspend() override;
  bool Resume() override;

  void DumpHeap(const char* filename) override;

 private:
  bool init_done_;

  // art::Dbg::SuspendVM
  void (*suspend_vm_fnc_)();
  // art::Dbg::ResumeVM
  void (*resume_vm_fnc_)();

  // art::hprof::DumpHeap()
  void (*dump_heap_func_)(const char *filename, int, bool);
};

} // namespace leak_monitor
} // namespace kwai

#endif //KOOM_HPROF_DUMP_BELOW_R_IMPL_H_
