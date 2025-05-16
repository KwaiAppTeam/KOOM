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

#ifndef KOOM_HPROF_DUMP_H
#define KOOM_HPROF_DUMP_H

#include <android-base/macros.h>

#include <memory>
#include <string>

namespace kwai {
namespace leak_monitor {

inline void FastExit(int exit_code) {
  _exit(exit_code);
}

class HprofDumpImpl;

class HprofDump {
 public:
  static HprofDump &GetInstance();

  void Initialize();

  // Avoid Any Not Necessary actions on the forked process
  pid_t SuspendAndFork();
  bool Resume();
  bool ResumeAndWait(pid_t pid);

  void DumpHeap(const char* filename);

 private:
  HprofDump();
  ~HprofDump() = default;
  DISALLOW_COPY_AND_ASSIGN(HprofDump);

 private:
  HprofDumpImpl &impl_;
};

}  // namespace leak_monitor
}  // namespace kwai

#endif  // KOOM_HPROF_DUMP_H
