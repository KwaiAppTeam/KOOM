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
