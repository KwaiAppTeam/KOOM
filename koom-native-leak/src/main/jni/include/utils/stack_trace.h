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

#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_STACK_TRACE_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_STACK_TRACE_H_

#include <stdint.h>
#include <stdlib.h>

#include <cstddef>
#include <iostream>

#include "constants.h"

#define FAST_UNWIND_TLS_INITIAL_EXEC \
  __thread __attribute__((tls_model("initial-exec")))

#ifndef KWAI_EXPORT
#define KWAI_EXPORT __attribute__((visibility("default")))
#endif

class StackTrace {
 public:
  static size_t FastUnwind(uintptr_t *buf, size_t num_entries);
};

#endif  // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_STACK_TRACE_H_
