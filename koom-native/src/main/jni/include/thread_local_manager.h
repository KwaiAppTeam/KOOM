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
 * Created by lbtrace on 2021.
 *
 */

#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_THREAD_ALLOC_MANAGER_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_THREAD_ALLOC_MANAGER_H_

#include "constants.h"

namespace kwai {
namespace leak_monitor {
struct AllocThreadInfo {
  uint32_t cursor;
  uintptr_t backtrace[kMaxBacktraceSize];
  char thread_name[kMaxThreadNameLen];
};

class ThreadLocalManager {
 public:
  static int InitOnce();
  static AllocThreadInfo *GetAllocThreadInfo();
 private:
  static pthread_key_t alloc_info_key_;
  static pthread_once_t alloc_info_once_;
};
} // namespace leak_monitor
} // namespace kwai
#endif // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_THREAD_ALLOC_MANAGER_H_
