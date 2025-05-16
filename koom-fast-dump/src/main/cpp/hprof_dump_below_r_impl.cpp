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

#include "hprof_dump_below_r_impl.h"
#include "defines.h"

#include <dlfcn.h>
#include <kwai_linker/kwai_dlfcn.h>
#include <log/kcheck.h>

#undef LOG_TAG
#define LOG_TAG "HprofDumpBelowRImpl"

namespace kwai {
namespace leak_monitor {

using namespace kwai::linker;

HprofDumpBelowRImpl &HprofDumpBelowRImpl::GetInstance() {
  static HprofDumpBelowRImpl instance;
  return instance;
}

HprofDumpBelowRImpl::HprofDumpBelowRImpl()
  : HprofDumpImpl(),
    init_done_(false),
    suspend_vm_fnc_(nullptr), resume_vm_fnc_(nullptr),
    dump_heap_func_(nullptr) {}

bool HprofDumpBelowRImpl::Initialize() {
  if (init_done_) {
    return true;
  }

  std::unique_ptr<void, decltype(&DlFcn::dlclose)>
      handle(DlFcn::dlopen("libart.so", RTLD_NOW), DlFcn::dlclose);
  KCHECKB(handle)

  suspend_vm_fnc_ =
      (void (*)())DlFcn::dlsym(handle.get(), "_ZN3art3Dbg9SuspendVMEv");
  KCHECKB(suspend_vm_fnc_)
  resume_vm_fnc_ =
      (void (*)())DlFcn::dlsym(handle.get(), "_ZN3art3Dbg8ResumeVMEv");
  KCHECKB(resume_vm_fnc_)

  dump_heap_func_ =
      (void (*)(const char *, int, bool))DlFcn::dlsym(handle.get(), "_ZN3art5hprof8DumpHeapEPKcib");
  KCHECKB(dump_heap_func_)

  init_done_ = true;
  return true;
}

bool HprofDumpBelowRImpl::Suspend() {
  KCHECKB(init_done_)
  suspend_vm_fnc_();
  return true;
}

bool HprofDumpBelowRImpl::Resume() {
  KCHECKB(init_done_)
  resume_vm_fnc_();
  return true;
}

void HprofDumpBelowRImpl::DumpHeap(const char* filename) {
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
