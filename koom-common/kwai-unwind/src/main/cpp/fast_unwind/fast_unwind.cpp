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

#include <fast_unwind/fast_unwind.h>
#include <kwai_util/kwai_macros.h>
#include <bits/pthread_types.h>
#include <log/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>

#define LOG_TAG "unwind"
#define LOG_NDEBUG 0

static FAST_UNWIND_TLS_INITIAL_EXEC uintptr_t stack_top = 0;
static FAST_UNWIND_TLS_INITIAL_EXEC pthread_once_t once_control_tls = PTHREAD_ONCE_INIT;

void fast_unwind_init() {
// TODO 先处理崩溃问题，之后这里会统一在外部初始化
//  if (getpid() == gettid() && stack_top == 0) {
//    LOG_ALWAYS_FATAL("main thread pthread_attr_t must be init in advance!");
//  }
  pthread_attr_t attr;
  pthread_getattr_np(pthread_self(), &attr);
  stack_top = (uintptr_t)(attr.stack_size + static_cast<char *>(attr.stack_base));
}

KWAI_EXPORT void fast_unwind_init_main_thread() {
  if (getpid() != gettid()) {
    LOG_ALWAYS_FATAL("%s must be called on main thread!", __FUNCTION__);
  }
  stack_top = -1;
  pthread_once(&once_control_tls, fast_unwind_init);
}

inline __attribute__((__always_inline__)) uintptr_t get_thread_stack_top() { return stack_top; }

KWAI_EXPORT size_t frame_pointer_unwind(uintptr_t *buf, size_t num_entries) {
  pthread_once(&once_control_tls, fast_unwind_init);
  struct frame_record {
    uintptr_t next_frame, return_addr;
  };

  auto begin = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
  auto end = get_thread_stack_top();

  stack_t ss;
  if (sigaltstack(nullptr, &ss) == 0 && (ss.ss_flags & SS_ONSTACK)) {
    end = reinterpret_cast<uintptr_t>(ss.ss_sp) + ss.ss_size;
  }

  size_t num_frames = 0;
  while (num_frames < num_entries) {
    auto *frame = reinterpret_cast<frame_record *>(begin);
    buf[num_frames++] = frame->return_addr;
    if (frame->next_frame < begin + sizeof(frame_record) || frame->next_frame >= end ||
        frame->next_frame % sizeof(void *) != 0) {
      break;
    }
    begin = frame->next_frame;
  }

  return num_frames;
}