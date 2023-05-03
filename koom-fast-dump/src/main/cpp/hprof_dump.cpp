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

#include <bionic/tls.h>
#include <dlfcn.h>
#include <hprof_dump.h>
#include <kwai_linker/kwai_dlfcn.h>
#include <log/kcheck.h>
#include <sys/prctl.h>
#include <wait.h>

#include <memory>

#undef LOG_TAG
#define LOG_TAG "HprofDump"

using namespace kwai::linker;

namespace kwai {
namespace leak_monitor {

HprofDump &HprofDump::GetInstance() {
  static HprofDump hprof_dump;
  return hprof_dump;
}

HprofDump::HprofDump() : init_done_(false), android_api_(0) {
  android_api_ = android_get_device_api_level();
}

void HprofDump::Initialize() {
  if (init_done_ || android_api_ < __ANDROID_API_L__) {
    return;
  }

  void *handle = kwai::linker::DlFcn::dlopen("libart.so", RTLD_NOW);
  KCHECKV(handle)

  if (android_api_ < __ANDROID_API_R__) {
    suspend_vm_fnc_ =
        (void (*)())DlFcn::dlsym(handle, "_ZN3art3Dbg9SuspendVMEv");
    KFINISHV_FNC(suspend_vm_fnc_, DlFcn::dlclose, handle)

    resume_vm_fnc_ = (void (*)())kwai::linker::DlFcn::dlsym(
        handle, "_ZN3art3Dbg8ResumeVMEv");
    KFINISHV_FNC(resume_vm_fnc_, DlFcn::dlclose, handle)
  } else if (android_api_ <= __ANDROID_API_S__) {
    // Over size for device compatibility
    ssa_instance_ = std::make_unique<char[]>(64);
    sgc_instance_ = std::make_unique<char[]>(64);

    ssa_constructor_fnc_ = (void (*)(void *, const char *, bool))DlFcn::dlsym(
        handle, "_ZN3art16ScopedSuspendAllC1EPKcb");
    KFINISHV_FNC(ssa_constructor_fnc_, DlFcn::dlclose, handle)

    ssa_destructor_fnc_ =
        (void (*)(void *))DlFcn::dlsym(handle, "_ZN3art16ScopedSuspendAllD1Ev");
    KFINISHV_FNC(ssa_destructor_fnc_, DlFcn::dlclose, handle)

    sgc_constructor_fnc_ =
        (void (*)(void *, void *, GcCause, CollectorType))DlFcn::dlsym(
            handle,
            "_ZN3art2gc23ScopedGCCriticalSectionC1EPNS_6ThreadENS0_"
            "7GcCauseENS0_13CollectorTypeE");
    KFINISHV_FNC(sgc_constructor_fnc_, DlFcn::dlclose, handle)

    sgc_destructor_fnc_ = (void (*)(void *))DlFcn::dlsym(
        handle, "_ZN3art2gc23ScopedGCCriticalSectionD1Ev");
    KFINISHV_FNC(sgc_destructor_fnc_, DlFcn::dlclose, handle)

    mutator_lock_ptr_ =
        (void **)DlFcn::dlsym(handle, "_ZN3art5Locks13mutator_lock_E");
    KFINISHV_FNC(mutator_lock_ptr_, DlFcn::dlclose, handle)

    exclusive_lock_fnc_ = (void (*)(void *, void *))DlFcn::dlsym(
        handle, "_ZN3art17ReaderWriterMutex13ExclusiveLockEPNS_6ThreadE");
    KFINISHV_FNC(exclusive_lock_fnc_, DlFcn::dlclose, handle)

    exclusive_unlock_fnc_ = (void (*)(void *, void *))DlFcn::dlsym(
        handle, "_ZN3art17ReaderWriterMutex15ExclusiveUnlockEPNS_6ThreadE");
    KFINISHV_FNC(exclusive_unlock_fnc_, DlFcn::dlclose, handle)
  }
  DlFcn::dlclose(handle);
  init_done_ = true;
}

pid_t HprofDump::SuspendAndFork() {
  KCHECKI(init_done_)

  if (android_api_ < __ANDROID_API_R__) {
    suspend_vm_fnc_();
  } else if (android_api_ <= __ANDROID_API_S__) {
    void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
    sgc_constructor_fnc_((void *)sgc_instance_.get(), self, kGcCauseHprof,
                         kCollectorTypeHprof);
    ssa_constructor_fnc_((void *)ssa_instance_.get(), LOG_TAG, true);
    // avoid deadlock with child process
    exclusive_unlock_fnc_(*mutator_lock_ptr_, self);
    sgc_destructor_fnc_((void *)sgc_instance_.get());
  }

  pid_t pid = fork();
  if (pid == 0) {
    // Set timeout for child process
    alarm(60);
    prctl(PR_SET_NAME, "forked-dump-process");
  }
  return pid;
}

bool HprofDump::ResumeAndWait(pid_t pid) {
  KCHECKB(init_done_)

  if (android_api_ < __ANDROID_API_R__) {
    resume_vm_fnc_();
  } else if (android_api_ <= __ANDROID_API_S__) {
    void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
    exclusive_lock_fnc_(*mutator_lock_ptr_, self);
    ssa_destructor_fnc_((void *)ssa_instance_.get());
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
    // if waitpid is interrupted by the signal,just call it again
    if (errno == EINTR){
      continue;
    }
    return false;
  }
}

}  // namespace leak_monitor
}  // namespace kwai