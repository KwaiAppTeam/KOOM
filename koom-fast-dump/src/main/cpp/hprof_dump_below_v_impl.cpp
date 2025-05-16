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

#include "hprof_dump_below_v_impl.h"

#include <wait.h>
#include <dlfcn.h>
#include <csetjmp>
#include <sys/prctl.h>
#include <sys/syscall.h>

#include <memory>

#include <kwai_linker/kwai_dlfcn.h>
#include <bionic/tls.h>
#include <log/kcheck.h>

#include "defines.h"

#undef LOG_TAG
#define LOG_TAG "HprofDumpBelowVImpl"

namespace kwai {
namespace leak_monitor {

using namespace kwai::linker;

HprofDumpBelowVImpl &HprofDumpBelowVImpl::GetInstance() {
  static HprofDumpBelowVImpl instance;
  return instance;
}

HprofDumpBelowVImpl::HprofDumpBelowVImpl()
    : HprofDumpImpl(),
      init_done_(false),
      ssa_constructor_fnc_(nullptr), ssa_destructor_fnc_(nullptr),
      sgc_constructor_fnc_(nullptr), sgc_destructor_fnc_(nullptr),
      mutator_lock_ptr_(nullptr), exclusive_lock_fnc_(nullptr), exclusive_unlock_fnc_(nullptr),
      dump_heap_func_(nullptr),
      ssa_instance_(nullptr), sgc_instance_(nullptr) {}

bool HprofDumpBelowVImpl::Initialize() {
  if (init_done_) {
    return true;
  }

  std::unique_ptr<void, decltype(&DlFcn::dlclose)>
      handle(DlFcn::dlopen("libart.so", RTLD_NOW), DlFcn::dlclose);
  KCHECKB(handle)

  // Over size for device compatibility
  ssa_instance_ = std::make_unique<char[]>(64);
  sgc_instance_ = std::make_unique<char[]>(64);

  ssa_constructor_fnc_ = (void (*)(void *, const char *, bool))DlFcn::dlsym(
      handle.get(), "_ZN3art16ScopedSuspendAllC1EPKcb");
  KCHECKB(ssa_constructor_fnc_)
  ssa_destructor_fnc_ =
      (void (*)(void *))DlFcn::dlsym(handle.get(), "_ZN3art16ScopedSuspendAllD1Ev");
  KCHECKB(ssa_destructor_fnc_)

  sgc_constructor_fnc_ = (void (*)(void *, void *, GcCause, CollectorType))DlFcn::dlsym(
      handle.get(),
      "_ZN3art2gc23ScopedGCCriticalSectionC1EPNS_6ThreadENS0_7GcCauseENS0_13CollectorTypeE");
  KCHECKB(sgc_constructor_fnc_)
  sgc_destructor_fnc_ =
      (void (*)(void *))DlFcn::dlsym(handle.get(), "_ZN3art2gc23ScopedGCCriticalSectionD1Ev");
  KCHECKB(sgc_destructor_fnc_)

  mutator_lock_ptr_ =
      (void **)DlFcn::dlsym(handle.get(), "_ZN3art5Locks13mutator_lock_E");
  KCHECKB(mutator_lock_ptr_)
  exclusive_lock_fnc_ =(void (*)(void *, void *))DlFcn::dlsym(
      handle.get(), "_ZN3art17ReaderWriterMutex13ExclusiveLockEPNS_6ThreadE");
  KCHECKB(exclusive_lock_fnc_)
  exclusive_unlock_fnc_ = (void (*)(void *, void *))DlFcn::dlsym(
      handle.get(), "_ZN3art17ReaderWriterMutex15ExclusiveUnlockEPNS_6ThreadE");
  KCHECKB(exclusive_unlock_fnc_)

  dump_heap_func_ =
      (void (*)(const char *, int, bool))DlFcn::dlsym(handle.get(), "_ZN3art5hprof8DumpHeapEPKcib");
  KCHECKB(dump_heap_func_)

  init_done_ = true;
  return true;
}

bool HprofDumpBelowVImpl::Suspend() {
  KCHECKB(init_done_)

  void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  sgc_constructor_fnc_((void *)sgc_instance_.get(), self, kGcCauseHprof, kCollectorTypeHprof);
  ssa_constructor_fnc_((void *)ssa_instance_.get(), LOG_TAG, true);
  // avoid deadlock with child process
  exclusive_unlock_fnc_(*mutator_lock_ptr_, self);
  sgc_destructor_fnc_((void *)sgc_instance_.get());

  return true;
}

bool HprofDumpBelowVImpl::Resume() {
  KCHECKB(init_done_)

  void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  exclusive_lock_fnc_(*mutator_lock_ptr_, self);
  ssa_destructor_fnc_((void *)ssa_instance_.get());

  return true;
}

void HprofDumpBelowVImpl::DumpHeap(const char* filename) {
  KCHECKV(init_done_)
  // If "direct_to_ddms" is true, the other arguments are ignored, and data is
  // sent directly to DDMS.
  // If "fd" is >= 0, the output will be written to that file descriptor.
  // Otherwise, "filename" is used to create an output file.
  // DumpHeap(const char* filename, int fd, bool direct_to_ddms)
  dump_heap_func_(filename, -1, false);
}

} // namespace leak_monitor
} // namespace kwai
