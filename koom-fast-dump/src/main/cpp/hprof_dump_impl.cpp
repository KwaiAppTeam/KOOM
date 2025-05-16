//
// Created by zack on 2025/1/26.
//

#include "hprof_dump_impl.h"

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <cerrno>

#include <log/kcheck.h>

#include "defines.h"

#include "hprof_dump.h"
#include "hprof_dump_below_r_impl.h"
#include "hprof_dump_below_v_impl.h"
#include "hprof_dump_v_impl.h"

#undef LOG_TAG
#define LOG_TAG "HprofDumpImpl"

namespace kwai {
namespace leak_monitor {

HprofDumpImpl &HprofDumpImpl::GetInstance(int android_api) {
  if (android_api < __ANDROID_API_R__) {
    return HprofDumpBelowRImpl::GetInstance();
  } else if (android_api < __ANDROID_API_V__) {
    return HprofDumpBelowVImpl::GetInstance();
  } else {
    // hprof_constructor_fnc_ symbol not exists on Android 14
    return HprofDumpVImpl::GetInstance();
  }
}

pid_t HprofDumpImpl::Fork() {
  return fork();
}

pid_t HprofDumpImpl::SuspendAndFork() {
  if (!Suspend()) {
    return -1;
  }

  pid_t pid = Fork();
  if (pid == 0) {
    // Set timeout for child process
    alarm(60);
    prctl(PR_SET_NAME, "forked-dump-process");
  }
  return pid;
}

bool HprofDumpImpl::ResumeAndWait(pid_t pid) {
  if (!Resume()) {
    return false;
  }

  int status;
  for (;;) {
    if (waitpid(pid, &status, 0) != -1) {
      if (!WIFEXITED(status)) {
        ALOGE("Child process %d exited with status %d, terminated by signal %d",
              pid, WEXITSTATUS(status), WTERMSIG(status));
        return false;
      }
      return true;
    }
    // 被信号中断调用的话，再发起一次waitpid调用即可
    if (errno == EINTR){
      continue;
    }
    return false;
  }
}

} // namespace leak_monitor
} // namespace kwai
