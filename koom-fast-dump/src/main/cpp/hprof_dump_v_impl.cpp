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

#include "hprof_dump_v_impl.h"

#include <dlfcn.h>

#include <kwai_linker/kwai_dlfcn.h>
#include <bionic/tls.h>
#include <log/kcheck.h>

#include <memory>

#include "defines.h"

#undef LOG_TAG
#define LOG_TAG "HprofDumpVImpl"

namespace kwai {
namespace leak_monitor {

using namespace kwai::linker;

HprofDumpVImpl &HprofDumpVImpl::GetInstance() {
  static HprofDumpVImpl instance;
  return instance;
}

HprofDumpVImpl::HprofDumpVImpl()
    : HprofDumpImpl(),
      init_done_(false),
      ssa_constructor_fnc_(nullptr), ssa_destructor_fnc_(nullptr),
      sgc_constructor_fnc_(nullptr), sgc_destructor_fnc_(nullptr),
      thread_list_lock_ptr_(nullptr), exclusive_lock_fnc_(nullptr), exclusive_unlock_fnc_(nullptr),
      dump_heap_func_(nullptr),
      ssa_instance_(nullptr), sgc_instance_(nullptr) {}

bool HprofDumpVImpl::Initialize() {
  if (init_done_) {
    return true;
  }

  std::unique_ptr<void, decltype(&DlFcn::dlclose_elf)>
      handle(DlFcn::dlopen_elf("libart.so", RTLD_NOW), DlFcn::dlclose_elf);
  KCHECKB(handle)

  ssa_constructor_fnc_ = (void (*)(void *, const char *, bool))DlFcn::dlsym_elf(
      handle.get(), "_ZN3art16ScopedSuspendAllC1EPKcb");
  KCHECKB(ssa_constructor_fnc_)
  ssa_destructor_fnc_ = (void (*)(void *))DlFcn::dlsym_elf(
      handle.get(), "_ZN3art16ScopedSuspendAllD1Ev");
  KCHECKB(ssa_destructor_fnc_)

  sgc_constructor_fnc_ = (void (*)(void *, void *, GcCause, CollectorType))DlFcn::dlsym_elf(
      handle.get(),
      "_ZN3art2gc23ScopedGCCriticalSectionC1EPNS_6ThreadENS0_7GcCauseENS0_13CollectorTypeE");
  KCHECKB(sgc_constructor_fnc_)
  sgc_destructor_fnc_ =
      (void (*)(void *))DlFcn::dlsym_elf(handle.get(), "_ZN3art2gc23ScopedGCCriticalSectionD1Ev");
  KCHECKB(sgc_destructor_fnc_)

  thread_list_lock_ptr_ =
      (void **)DlFcn::dlsym_elf(handle.get(), "_ZN3art5Locks17thread_list_lock_E");
  KCHECKB(thread_list_lock_ptr_)
  exclusive_lock_fnc_ = (void (*)(void *, void *))DlFcn::dlsym_elf(
      handle.get(), "_ZN3art5Mutex13ExclusiveLockEPNS_6ThreadE");
  KCHECKB(exclusive_lock_fnc_)
  exclusive_unlock_fnc_ = (void (*)(void *, void *))DlFcn::dlsym_elf(
      handle.get(), "_ZN3art5Mutex15ExclusiveUnlockEPNS_6ThreadE");
  KCHECKB(exclusive_unlock_fnc_)

  dump_heap_func_ =
      (void (*)(const char *, int, bool))DlFcn::dlsym_elf(handle.get(), "_ZN3art5hprof8DumpHeapEPKcib");
  KCHECKB(dump_heap_func_)

  init_done_ = true;
  return true;
}

// class ScopedSuspendAll : public ValueObject {
// };
//
// class ValueObject {
// };
class ScopedSuspendAll {
 public:
  ScopedSuspendAll(const char *cause, bool long_suspend) {
    HprofDumpVImpl::GetInstance().ssa_constructor_fnc_(this, cause, long_suspend);
  }

  ~ScopedSuspendAll() {
    HprofDumpVImpl::GetInstance().ssa_destructor_fnc_(this);
  }

 private:
  // Over size for device compatibility
  [[maybe_unused]] char placeholder_[64] = {0};
};

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
class ScopedGCCriticalSection {
 public:
  ScopedGCCriticalSection(void *self, GcCause cause, CollectorType collector_type) {
    HprofDumpVImpl::GetInstance().sgc_constructor_fnc_(this, self, cause, collector_type);
  }

  ~ScopedGCCriticalSection() {
    HprofDumpVImpl::GetInstance().sgc_destructor_fnc_(this);
  }

 private:
  // Over size for device compatibility
  [[maybe_unused]] char placeholder_[64] = {0};
};

class MutexLock {
 public:
  MutexLock(void *self, void **mu_ptr) : self_(self), mu_ptr_(mu_ptr) {
    HprofDumpVImpl::GetInstance().exclusive_lock_fnc_(*mu_ptr_, self_);
  }

  ~MutexLock() {
    HprofDumpVImpl::GetInstance().exclusive_unlock_fnc_(*mu_ptr_, self_);
  }

 private:
  void *self_;
  void **mu_ptr_;
};

bool HprofDumpVImpl::Suspend() {
  KCHECKB(init_done_)

  // see
  // perfetto ForkAndRun: https://cs.android.com/android/platform/superproject/main/+/main:art/perfetto_hprof/perfetto_hprof.cc;l=985;
  // hprof DumpHeap: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/hprof/hprof.cc;l=1616
  void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  sgc_instance_ = std::make_unique<ScopedGCCriticalSection>(self, kGcCauseHprof, kCollectorTypeHprof);
  ssa_instance_ = std::make_unique<ScopedSuspendAll>(LOG_TAG, true);

  return true;
}

pid_t HprofDumpVImpl::Fork() {
  KCHECKI(init_done_)

  // see https://cs.android.com/android/_/android/platform/art/+/5656cd41481ab03eb6df3aec7eda296ebbef667b
  void *self = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  MutexLock lk(self, thread_list_lock_ptr_);
  return HprofDumpImpl::Fork();
}

bool HprofDumpVImpl::Resume() {
  KCHECKB(init_done_)

  ssa_instance_.reset();
  sgc_instance_.reset();

  return true;
}

void HprofDumpVImpl::DumpHeap(const char* filename) {
  KCHECKV(init_done_)

  Resume();
  // If "direct_to_ddms" is true, the other arguments are ignored, and data is
  // sent directly to DDMS.
  // If "fd" is >= 0, the output will be written to that file descriptor.
  // Otherwise, "filename" is used to create an output file.
  // DumpHeap(const char* filename, int fd, bool direct_to_ddms)
  dump_heap_func_(filename, -1, false);
}

} // namespace leak_monitor
} // namespace kwai
