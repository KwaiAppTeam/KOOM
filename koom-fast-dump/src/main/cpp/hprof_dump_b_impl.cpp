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

#include "hprof_dump_b_impl.h"

#include <dlfcn.h>

#include <memory>

#include <bionic/tls.h>
#include <kwai_linker/kwai_dlfcn.h>
#include <log/kcheck.h>

#include "defines.h"

#undef LOG_TAG
#define LOG_TAG "HprofDumpBImpl"

namespace kwai {
namespace leak_monitor {

using namespace kwai::linker;

HprofDumpBImpl &HprofDumpBImpl::GetInstance() {
  static HprofDumpBImpl instance;
  return instance;
}

HprofDumpBImpl::HprofDumpBImpl()
    : HprofDumpImpl(),
      init_done_(false),
      ssa_constructor_fnc_(nullptr),
      ssa_destructor_fnc_(nullptr),
      sgc_constructor_fnc_(nullptr),
      sgc_destructor_fnc_(nullptr),
      thread_list_lock_ptr_(nullptr),
      exclusive_lock_fnc_(nullptr),
      exclusive_unlock_fnc_(nullptr),
      dump_heap_func_(nullptr),
      ssa_instance_(nullptr),
      sgc_instance_(nullptr) {}

bool HprofDumpBImpl::Initialize() {
  if (init_done_) {
    return true;
  }

  std::unique_ptr<void, decltype(&DlFcn::dlclose_elf)> handle(
      DlFcn::dlopen_elf("libart.so", RTLD_NOW), DlFcn::dlclose_elf);
  KCHECKB(handle)

  ssa_constructor_fnc_ = reinterpret_cast<void (*)(void *, const char *, bool)>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art16ScopedSuspendAllC1EPKcb"));
  KCHECKB(ssa_constructor_fnc_)
  ssa_destructor_fnc_ = reinterpret_cast<void (*)(void *)>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art16ScopedSuspendAllD1Ev"));
  KCHECKB(ssa_destructor_fnc_)

  sgc_constructor_fnc_ = reinterpret_cast<void (*)(void *, void *, GcCause, CollectorType)>(
      DlFcn::dlsym_elf(handle.get(),
                       "_ZN3art2gc23ScopedGCCriticalSectionC1EPNS_6ThreadENS0_7GcCauseENS0_13CollectorTypeE"));
  KCHECKB(sgc_constructor_fnc_)
  sgc_destructor_fnc_ = reinterpret_cast<void (*)(void *)>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art2gc23ScopedGCCriticalSectionD1Ev"));
  KCHECKB(sgc_destructor_fnc_)

  thread_list_lock_ptr_ = reinterpret_cast<void **>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art5Locks17thread_list_lock_E"));
  KCHECKB(thread_list_lock_ptr_)
  exclusive_lock_fnc_ = reinterpret_cast<void (*)(void *, void *)>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art5Mutex13ExclusiveLockEPNS_6ThreadE"));
  KCHECKB(exclusive_lock_fnc_)
  exclusive_unlock_fnc_ = reinterpret_cast<void (*)(void *, void *)>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art5Mutex15ExclusiveUnlockEPNS_6ThreadE"));
  KCHECKB(exclusive_unlock_fnc_)

  dump_heap_func_ = reinterpret_cast<void (*)(const char *, int, bool)>(
      DlFcn::dlsym_elf(handle.get(), "_ZN3art5hprof8DumpHeapEPKcib"));
  KCHECKB(dump_heap_func_)

  init_done_ = true;
  return true;
}

class ScopedSuspendAllB {
 public:
  ScopedSuspendAllB(const char *cause, bool long_suspend) {
    HprofDumpBImpl::GetInstance().ssa_constructor_fnc_(this, cause, long_suspend);
  }

  ~ScopedSuspendAllB() {
    HprofDumpBImpl::GetInstance().ssa_destructor_fnc_(this);
  }

 private:
  [[maybe_unused]] char placeholder_[64] = {0};
};

class ScopedGCCriticalSectionB {
 public:
  ScopedGCCriticalSectionB(void *self, GcCause cause, CollectorType collector_type) {
    HprofDumpBImpl::GetInstance().sgc_constructor_fnc_(this, self, cause, collector_type);
  }

  ~ScopedGCCriticalSectionB() {
    HprofDumpBImpl::GetInstance().sgc_destructor_fnc_(this);
  }

 private:
  [[maybe_unused]] char placeholder_[64] = {0};
};

class MutexLockB {
 public:
  MutexLockB(void *self, void **mu_ptr) : self_(self), mu_ptr_(mu_ptr) {
    HprofDumpBImpl::GetInstance().exclusive_lock_fnc_(*mu_ptr_, self_);
  }

  ~MutexLockB() {
    HprofDumpBImpl::GetInstance().exclusive_unlock_fnc_(*mu_ptr_, self_);
  }

 private:
  void *self_;
  void **mu_ptr_;
};

bool HprofDumpBImpl::Suspend() {
  KCHECKB(init_done_)

  void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  sgc_instance_ = std::make_unique<ScopedGCCriticalSectionB>(
      self, kGcCauseHprof, kCollectorTypeHprof);
  ssa_instance_ = std::make_unique<ScopedSuspendAllB>(LOG_TAG, true);

  return true;
}

pid_t HprofDumpBImpl::Fork() {
  KCHECKI(init_done_)

  // Android 16 ART's perfetto_hprof::ForkUnderThreadListLock() holds this lock around fork.
  void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  MutexLockB thread_list_lock(self, thread_list_lock_ptr_);
  return HprofDumpImpl::Fork();
}

bool HprofDumpBImpl::Resume() {
  KCHECKB(init_done_)

  ssa_instance_.reset();
  sgc_instance_.reset();

  return true;
}

void HprofDumpBImpl::DumpHeap(const char *filename) {
  KCHECKV(init_done_)

  Resume();
  dump_heap_func_(filename, -1, false);
}

}  // namespace leak_monitor
}  // namespace kwai
