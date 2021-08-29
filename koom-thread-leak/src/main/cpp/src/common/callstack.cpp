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

#include "callstack.h"

#include <dlfcn.h>
#include <kwai_linker/kwai_dlfcn.h>

#include "bionic/tls.h"
#include "bionic/tls_defines.h"

namespace koom {

const char *callstack_tag = "koom-callstack";

//静态变量初始化
pthread_key_t CallStack::pthread_key_self;
dump_java_stack_above_o_ptr CallStack::dump_java_stack_above_o;
dump_java_stack_ptr CallStack::dump_java_stack;

std::atomic<bool> CallStack::disableJava;
std::atomic<bool> CallStack::disableNative;
std::mutex CallStack::dumpJavaLock;

std::atomic<bool> CallStack::inSymbolize;

unwindstack::UnwinderFromPid *CallStack::unwinder;

void CallStack::Init() {
  if (koom::Util::AndroidApi() < __ANDROID_API_L__) {
    koom::Log::error(callstack_tag, "android api < __ANDROID_API_L__");
    return;
  }
  void *handle =
      kwai::linker::DlFcn::dlopen("libart.so", RTLD_LAZY | RTLD_LOCAL);
  if (koom::Util::AndroidApi() >= __ANDROID_API_O__) {
    dump_java_stack_above_o = reinterpret_cast<
        dump_java_stack_above_o_ptr>(kwai::linker::DlFcn::dlsym(
        handle,
        "_ZNK3art6Thread13DumpJavaStackERNSt3__113basic_ostreamIcNS1_11char_"
        "traitsIcEEEEbb"));
    if (dump_java_stack_above_o == nullptr) {
      koom::Log::error(callstack_tag, "dump_java_stack_above_o is null");
    }
  } else if (koom::Util::AndroidApi() >= __ANDROID_API_L__) {
    dump_java_stack = reinterpret_cast<
        dump_java_stack_ptr>(kwai::linker::DlFcn::dlsym(
        handle,
        "_ZNK3art6Thread13DumpJavaStackERNSt3__113basic_ostreamIcNS1_11char_"
        "traitsIcEEEE"));
    if (dump_java_stack == nullptr) {
      koom::Log::error(callstack_tag, "dump_java_stack is null");
    }
  }

  if (koom::Util::AndroidApi() < __ANDROID_API_N__) {
    auto *pthread_key_self_art = (pthread_key_t *)kwai::linker::DlFcn::dlsym(
        handle, "_ZN3art6Thread17pthread_key_self_E");
    if (pthread_key_self_art != nullptr) {
      pthread_key_self = reinterpret_cast<pthread_key_t>(*pthread_key_self_art);
    } else {
      koom::Log::error(callstack_tag, "pthread_key_self_art is null");
    }
  }

  kwai::linker::DlFcn::dlclose(handle);
}

void *CallStack::GetCurrentThread() {
  if (koom::Util::AndroidApi() >= __ANDROID_API_N__) {
    return __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  }
  if (koom::Util::AndroidApi() >= __ANDROID_API_L__) {
    return pthread_getspecific(pthread_key_self);
  }
  koom::Log::info(callstack_tag, "GetCurrentThread return");
  return nullptr;
}

void CallStack::JavaStackTrace(void *thread, std::ostream &os) {
  if (disableJava.load()) {
    os << "no java stack when dumping";
    return;
  }
  if (dumpJavaLock.try_lock()) {
    if (koom::Util::AndroidApi() >= __ANDROID_API_O__) {
      //不dump locks，有稳定性问题
      dump_java_stack_above_o(thread, os, true, false);
    } else if (koom::Util::AndroidApi() >= __ANDROID_API_L__) {
      dump_java_stack(thread, os);
    }
    dumpJavaLock.unlock();
  }
}

size_t CallStack::FastUnwind(uintptr_t *buf, size_t num_entries) {
  if (disableNative.load()) {
    return 0;
  }
  return frame_pointer_unwind(buf, num_entries);
}

std::string CallStack::SymbolizePc(uintptr_t pc, int index) {
  if (inSymbolize.load()) return "";
  inSymbolize = true;

  if (unwinder == nullptr) {
    unwinder = new unwindstack::UnwinderFromPid(
        koom::Constant::kMaxCallStackDepth, getpid(),
        unwindstack::Regs::CurrentArch());
    unwinder->Init();
    unwinder->SetDisplayBuildID(true);
    unwinder->SetRegs(unwindstack::Regs::CreateFromLocal());
  }
  std::string format;
  unwindstack::FrameData data = unwinder->BuildFrameFromPcOnly(pc);
  if (data.map_name.find("libkoom-thread") != std::string::npos) {
    inSymbolize = false;
    return "";
  }
  data.num = index;
  format = unwinder->FormatFrame(data);

  inSymbolize = false;
  return format;
}

void CallStack::DisableJava() { disableJava = true; }

void CallStack::DisableNative() { disableNative = true; }
}  // namespace koom
