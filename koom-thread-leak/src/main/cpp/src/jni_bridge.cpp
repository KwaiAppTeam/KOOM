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

#include <jni.h>

#include "common/callstack.h"
#include "koom.h"

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  koom::Log::info("koom-thread", "JNI_OnLoad");
  JNIEnv *env = nullptr;
  if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }
  koom::Init(vm, env);
  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_disableJavaStack(
    JNIEnv *env, jclass jObject) {
  koom::CallStack::DisableJava();
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_disableNativeStack(
    JNIEnv *env, jclass jObject) {
  koom::CallStack::DisableNative();
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_start(
    JNIEnv *env, jclass obj) {
  koom::Log::info("koom-thread", "start");
  koom::Start();
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_refresh(
    JNIEnv *env, jclass obj) {
  koom::Refresh();
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_stop(
    JNIEnv *env, jclass obj) {
  koom::Stop();
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_setThreadLeakDelay(
    JNIEnv *env, jclass thiz, jlong delay) {
  koom::threadLeakDelay = delay;
}

JNIEXPORT void JNICALL
Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_enableNativeLog(
    JNIEnv *env, jclass jObject) {
  koom::Log::log_enable = true;
}
}
