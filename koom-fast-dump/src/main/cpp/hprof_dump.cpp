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

#include "hprof_dump.h"

#include "hprof_dump_impl.h"

namespace kwai {
namespace leak_monitor {

HprofDump &HprofDump::GetInstance() {
  static HprofDump hprof_dump;
  return hprof_dump;
}

HprofDump::HprofDump() : impl_(HprofDumpImpl::GetInstance(android_get_device_api_level())) {}

void HprofDump::Initialize() {
  impl_.Initialize();
}

pid_t HprofDump::SuspendAndFork() {
  return impl_.SuspendAndFork();
}

bool HprofDump::Resume() {
  return impl_.Resume();
}

bool HprofDump::ResumeAndWait(pid_t pid) {
  return impl_.ResumeAndWait(pid);
}

void HprofDump::DumpHeap(const char* filename) {
  return impl_.DumpHeap(filename);
}

}  // namespace leak_monitor
}  // namespace kwai