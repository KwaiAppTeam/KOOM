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
 *
 * Created by wangzefeng <wangzefeng@kuaishou.com> on 2025.
 *
 */

#include "hprof_dump_impl.h"

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <signal.h>

#include <log/kcheck.h>

#include "defines.h"

#include "hprof_dump.h"
#include "hprof_dump_below_r_impl.h"
#include "hprof_dump_below_v_impl.h"
#include "hprof_dump_v_impl.h"
#include "hprof_dump_b_impl.h"
#include "hprof_dump_c_impl.h"

#undef LOG_TAG
#define LOG_TAG "HprofDumpImpl"

namespace kwai {
namespace leak_monitor {

namespace {

constexpr int kChildSignalExitStatusBase = 128;

constexpr int kChildFatalSignals[] = {
    SIGSEGV,
    SIGABRT,
    SIGBUS,
    SIGILL,
    SIGFPE,
    SIGTRAP,
    SIGSYS,
    SIGSTKFLT,
};

void ChildFatalSignalHandler(int signal_number, siginfo_t*, void*) {
  // Keep this handler async-signal-safe; the parent observes the exit status.
  _exit(kChildSignalExitStatusBase + signal_number);
}

void InstallChildFatalSignalHandlers() {
  struct sigaction action = {};
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = ChildFatalSignalHandler;
  action.sa_flags = SA_SIGINFO;
  for (const int signal_number : kChildFatalSignals) {
    (void)sigaction(signal_number, &action, nullptr);
  }
}

}  // namespace

HprofDumpImpl &HprofDumpImpl::GetInstance(int android_api) {
  if (android_api < __ANDROID_API_R__) {
    return HprofDumpBelowRImpl::GetInstance();
  }
  if (android_api < __ANDROID_API_V__) {
    return HprofDumpBelowVImpl::GetInstance();
  }
  if (android_api == __ANDROID_API_V__) {
    return HprofDumpVImpl::GetInstance();
  }
  if (android_api == __ANDROID_API_B__) {
    return HprofDumpBImpl::GetInstance();
  }
  return HprofDumpCImpl::GetInstance();
}

pid_t HprofDumpImpl::Fork() {
  return fork();
}

pid_t HprofDumpImpl::SuspendAndFork() {
  if (!Suspend()) {
    return -1;
  }

  pid_t pid = Fork();
  if (pid == -1) {
    const int fork_errno = errno;
    ALOGE("Fork dump process failed: %s", std::strerror(fork_errno));
    Resume();
    return -1;
  }
  if (pid == 0) {
    // Best effort: continue even if a signal handler cannot be installed.
    InstallChildFatalSignalHandlers();
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
      if (WIFSIGNALED(status)) {
        ALOGE("Child process %d terminated by signal %d", pid, WTERMSIG(status));
        return false;
      }
      if (!WIFEXITED(status)) {
        ALOGE("Child process %d exited with unexpected wait status %d", pid, status);
        return false;
      }
      if (WEXITSTATUS(status) != 0) {
        ALOGE("Child process %d exited with status %d", pid, WEXITSTATUS(status));
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
