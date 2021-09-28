/*
 * Copyright (c) 2020. Kwai, Inc. All rights reserved.
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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2020.
 *
 */

#pragma once

#include <sys/cdefs.h>
#include <backtrace/backtrace_constants.h>
#include <cstdint>

__BEGIN_DECLS

extern "C" size_t android_unsafe_frame_pointer_chase(uintptr_t *buf, size_t num_entries)
    __attribute__((weak));

#define FAST_UNWIND_TLS_INITIAL_EXEC __thread __attribute__((tls_model("initial-exec")))

uintptr_t get_thread_stack_top();
/* must initialize pthread_attr_t attr in main thread to avoid open@LIBC recursive */
void fast_unwind_init_main_thread();

size_t frame_pointer_unwind(uintptr_t *buf, size_t num_entries);

inline __attribute__((__always_inline__)) size_t fast_unwind(uintptr_t *buf, size_t num_entries) {
  if (android_unsafe_frame_pointer_chase) {
    return android_unsafe_frame_pointer_chase(buf, num_entries);
  }
  return frame_pointer_unwind(buf, num_entries);
}

__END_DECLS