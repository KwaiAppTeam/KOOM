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

#include "koom.h"

#include <jni.h>

#include "common/callstack.h"
#include "common/util.h"
#include "thread/hook_looper.h"
#include "thread/thread_hook.h"

namespace koom {

int Util::android_api;
bool Log::log_enable = false;

JavaVM *java_vm_;
jclass native_handler_class;
jmethodID java_callback_method;
std::atomic<bool> isRunning;
HookLooper *sHookLooper;
long threadLeakDelay;

void Init(JavaVM *vm, _JNIEnv *env) {
  java_vm_ = vm;
  auto clazz = env->FindClass(
      "com/kwai/performance/overhead/thread/monitor/NativeHandler");
  native_handler_class = static_cast<jclass>(env->NewGlobalRef(clazz));
  java_callback_method = env->GetStaticMethodID(
      native_handler_class, "nativeReport", "(Ljava/lang/String;)V");
  Util::Init();
  Log::info("koom", "Init, android api:%d", Util::AndroidApi());
  CallStack::Init();
}

void Start() {
  if (isRunning) {
    return;
  }
  // 初始化数据
  delete sHookLooper;
  sHookLooper = new HookLooper();
  koom::ThreadHooker::Start();
  isRunning = true;
}

void Stop() {
  isRunning = false;
  koom::ThreadHooker::Stop();
  sHookLooper->quit();
}

void Refresh() {
  auto info = new SimpleHookInfo(Util::CurrentTimeNs());
  sHookLooper->post(ACTION_REFRESH, info);
}

JNIEnv *GetEnv(bool doAttach) {
  JNIEnv *env = nullptr;
  int status = java_vm_->GetEnv((void **)&env, JNI_VERSION_1_6);
  if ((status == JNI_EDETACHED || env == nullptr) && doAttach) {
    status = java_vm_->AttachCurrentThread(&env, nullptr);
    if (status < 0) {
      env = nullptr;
    }
  }
  return env;
}

void JavaCallback(const char *value, bool doAttach) {
  JNIEnv *env = GetEnv(doAttach);
  if (env != nullptr && value != nullptr) {
    Log::error("koom", "JavaCallback %d", strlen(value));
    jstring string_value = env->NewStringUTF(value);
    env->CallStaticVoidMethod(native_handler_class, java_callback_method,
                              string_value);
    Log::info("koom", "JavaCallback finished");
  } else {
    Log::info("koom", "JavaCallback fail no JNIEnv");
  }
}

}  // namespace koom