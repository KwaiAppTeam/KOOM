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

#ifndef APM_KOOM_H
#define APM_KOOM_H

#include <jni.h>

#include "thread/hook_looper.h"

namespace koom {

extern JavaVM *java_vm_;

extern jclass native_handler_class;

extern jmethodID java_callback_method;

extern HookLooper *sHookLooper;

extern std::atomic<bool> isRunning;

extern int64_t threadLeakDelay;

extern void Init(JavaVM *vm, JNIEnv *p_env);

extern void Start();

extern void Stop();

extern void Report(JNIEnv *env, jobject obj, jstring type);

extern void Refresh();

JNIEnv *GetEnv(bool doAttach = true);

void JavaCallback(const char *value, bool doAttach = true);
}  // namespace koom

#endif  // APM_KOOM_H
