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

#ifndef KOOM_KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_THREAD_LOOP_ITEM_H_
#define KOOM_KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_THREAD_LOOP_ITEM_H_
namespace koom {
enum HookAction {
  ACTION_ADD_THREAD,
  ACTION_START_THREAD,
  ACTION_JOIN_THREAD,
  ACTION_EXIT_THREAD,
  ACTION_DETACH_THREAD,
  ACTION_INIT,
  ACTION_REFRESH,
  ACTION_SET_NAME,
};

class ThreadCreateArg {
 public:
  int64_t time;
  int64_t stack_time;
  std::ostringstream java_stack;
  uintptr_t pc[koom::Constant::kMaxCallStackDepth]{};
  ThreadCreateArg() {}
  ~ThreadCreateArg() { memset(pc, 0, sizeof(pc)); }
};

struct SimpleHookInfo {
  long long time;
  SimpleHookInfo(long long time) { this->time = time; }
};
struct HookInfo {
  pthread_t thread_id;
  long long time;
  HookInfo(pthread_t threadId, long long time) {
    this->thread_id = threadId;
    this->time = time;
  }
};
struct HookExitInfo {
  pthread_t thread_id;
  long long time;
  int tid;
  std::string threadName;
  HookExitInfo(pthread_t threadId, int tid, char *threadName, long long time) {
    this->thread_id = threadId;
    this->tid = tid;
    this->threadName.assign(threadName);
    this->time = time;
  }
};

struct HookAddInfo {
 public:
  int tid;
  int64_t time;
  pthread_t pthread;
  bool is_thread_detached;
  ThreadCreateArg *create_arg;

  HookAddInfo(int tid, long long time, pthread_t pthread, bool isThreadDetached,
              ThreadCreateArg *thread_create_arg) {
    this->tid = tid;
    this->time = time;
    this->pthread = pthread;
    this->is_thread_detached = isThreadDetached;
    this->create_arg = thread_create_arg;
  };
};
}  // namespace koom
#endif  // KOOM_KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_THREAD_LOOP_ITEM_H_
