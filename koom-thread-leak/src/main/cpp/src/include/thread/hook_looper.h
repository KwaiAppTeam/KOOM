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

#ifndef KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_INCLUDE_THREAD_HOOK_LOOPER_H_
#define KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_INCLUDE_THREAD_HOOK_LOOPER_H_

#include <common/looper.h>
#include <jni.h>
#include <thread/thread_holder.h>

#include <map>
namespace koom {
class HookLooper : public looper {
 public:
  koom::ThreadHolder *holder;
  HookLooper();
  ~HookLooper();
  void handle(int what, void *data);
  void post(int what, void *data);
};
}  // namespace koom
#endif  // KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_INCLUDE_THREAD_HOOK_LOOPER_H_
