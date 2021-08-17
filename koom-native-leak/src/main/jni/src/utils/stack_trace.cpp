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

#include "utils/stack_trace.h"

static const uint32_t kT32InstrLen = 2;
static const uint32_t kA32InstrLen = 4;
static const uint32_t kA64InstrLen = 4;
static FAST_UNWIND_TLS_INITIAL_EXEC uintptr_t stack_top = 0;
static FAST_UNWIND_TLS_INITIAL_EXEC pthread_once_t once_control_tls =
    PTHREAD_ONCE_INIT;

inline __attribute__((__always_inline__)) uintptr_t get_thread_stack_top() {
  return stack_top;
}

struct frame_record {
  uintptr_t next_frame, return_addr;
};

void fast_unwind_init() {
  pthread_attr_t attr;
  pthread_getattr_np(pthread_self(), &attr);
  stack_top =
      (uintptr_t)(attr.stack_size + static_cast<char *>(attr.stack_base));
}

static inline uintptr_t GetAdjustPC(uintptr_t pc) {
#if defined(__aarch64__) || defined(__arm__)
  if (pc < kA64InstrLen) {
    return 0;
  }

#if defined(__aarch64__)
  if (pc > kA64InstrLen) {
    pc -= kA64InstrLen;
  }
#else
  if (pc & 1) {
    pc -= kT32InstrLen;
  } else {
    pc -= kA32InstrLen;
  }
#endif
#endif
  return pc;
}

KWAI_EXPORT size_t StackTrace::FastUnwind(uintptr_t *buf, size_t num_entries) {
  pthread_once(&once_control_tls, fast_unwind_init);
  auto begin = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
  auto end = get_thread_stack_top();
  stack_t ss;
  if (sigaltstack(nullptr, &ss) == 0 && (ss.ss_flags & SS_ONSTACK)) {
    end = reinterpret_cast<uintptr_t>(ss.ss_sp) + ss.ss_size;
  }
  size_t num_frames = 0;
  while (num_frames < kMaxBacktraceSize) {
    auto *frame = reinterpret_cast<frame_record *>(begin);
    if (num_frames < num_entries) {
      buf[num_frames] = GetAdjustPC(frame->return_addr);
    }
    ++num_frames;
    if (frame->next_frame < begin + sizeof(frame_record) ||
        frame->next_frame >= end || frame->next_frame % sizeof(void *) != 0) {
      break;
    }
    begin = frame->next_frame;
  }
  return num_frames;
}