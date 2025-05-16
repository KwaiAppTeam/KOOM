//
// Created by zack on 2025/1/26.
//

#ifndef KOOM_HPROF_DUMP_IMPL_H_
#define KOOM_HPROF_DUMP_IMPL_H_

#include <sys/types.h>

namespace kwai {
namespace leak_monitor {

class HprofDumpImpl {
 public:
  static HprofDumpImpl &GetInstance(int android_api);

 public:
  virtual ~HprofDumpImpl() {};

 public:
  virtual bool Initialize() = 0;

 public:
  virtual bool Suspend() = 0;
  virtual pid_t Fork();
  virtual bool Resume() = 0;

  virtual void DumpHeap(const char* filename) = 0;

 public:
  // Avoid Any Not Necessary actions on the forked process
  pid_t SuspendAndFork();
  bool ResumeAndWait(pid_t pid);
};

} // namespace leak_monitor
} // namespace kwai

#endif //KOOM_HPROF_DUMP_IMPL_H_
