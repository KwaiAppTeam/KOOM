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
 * Created by shenvsv on 2021.
 *
 */

#ifndef APM_CALLSTACK_H
#define APM_CALLSTACK_H

#include <fast_unwind/fast_unwind.h>
#include <unistd.h>
#include <unwindstack/Unwinder.h>

#include <ostream>
#include <sstream>

#include "constant.h"
#include "util.h"

namespace koom {

using dump_java_stack_above_o_ptr = void (*)(void *, std::ostream &os, bool,
                                             bool);
using dump_java_stack_ptr = void (*)(void *, std::ostream &os);

class CallStack {
  enum Type { java, native };

 private:
  static dump_java_stack_above_o_ptr dump_java_stack_above_o;
  static dump_java_stack_ptr dump_java_stack;
  static pthread_key_t pthread_key_self;
  static unwindstack::UnwinderFromPid *unwinder;
  static std::atomic<bool> inSymbolize;

  static std::atomic<bool> disableJava;
  static std::atomic<bool> disableNative;

  static std::mutex dumpJavaLock;

 public:
  static void Init();

  static void DisableJava();

  static void DisableNative();

  static void JavaStackTrace(void *thread, std::ostream &os);

  static size_t FastUnwind(uintptr_t *buf, size_t num_entries);

  static std::string SymbolizePc(uintptr_t pc, int index);

  static void *GetCurrentThread();
};

}  // namespace koom

#endif  // APM_CALLSTACK_H
