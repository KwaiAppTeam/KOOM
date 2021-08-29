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

#include "thread_hook.h"

#include <dlopencb.h>
#include <kwai_util/kwai_macros.h>
#include <link.h>
#include <sys/prctl.h>
#include <syscall.h>
#include <xhook.h>

namespace koom {

const char *thread_tag = "thread-hook";

const char *ignore_libs[] = {"koom-thread", "liblog.so", "perfd", "memtrack"};

static bool IsLibIgnored(const std::string &lib) {
  for (const auto &ignoreLib : ignore_libs) {
    if (lib.find(ignoreLib) != -1) {
      return true;
    }
  }
  return false;
}

int Callback(struct dl_phdr_info *info, size_t size, void *data) {
  auto *libs = static_cast<std::set<std::string> *>(data);
  libs->insert(info->dlpi_name);
  return 0;
}

void ThreadHooker::InitHook() {
  koom::Log::info(thread_tag, "HookSo init hook");
  std::set<std::string> libs;
  DlopenCb::GetInstance().GetLoadedLibs(libs);
  HookLibs(libs, Constant::kDlopenSourceInit);
  DlopenCb::GetInstance().AddCallback(DlopenCallback);
}

void ThreadHooker::DlopenCallback(std::set<std::string> &libs, int source,
                                  std::string &source_lib) {
  HookLibs(libs, source);
}

void ThreadHooker::HookLibs(std::set<std::string> &libs, int source) {
  koom::Log::info(thread_tag, "HookSo lib size %d", libs.size());
  if (libs.empty()) {
    return;
  }
  bool hooked = false;
  pthread_mutex_lock(&DlopenCb::hook_mutex);
  xhook_clear();
  for (const auto &lib : libs) {
    hooked |= ThreadHooker::RegisterSo(lib, source);
  }
  if (hooked) {
    int result = xhook_refresh(0);
    koom::Log::info(thread_tag, "HookSo lib Refresh result %d", result);
  }
  pthread_mutex_unlock(&DlopenCb::hook_mutex);
}

bool ThreadHooker::RegisterSo(const std::string &lib, int source) {
  if (IsLibIgnored(lib)) {
    return false;
  }
  auto lib_ctr = lib.c_str();
  koom::Log::info(thread_tag, "HookSo %d %s", source, lib_ctr);
  xhook_register(lib_ctr, "pthread_create",
                 reinterpret_cast<void *>(HookThreadCreate), nullptr);
  xhook_register(lib_ctr, "pthread_detach",
                 reinterpret_cast<void *>(HookThreadDetach), nullptr);
  xhook_register(lib_ctr, "pthread_join",
                 reinterpret_cast<void *>(HookThreadJoin), nullptr);
  xhook_register(lib_ctr, "pthread_exit",
                 reinterpret_cast<void *>(HookThreadExit), nullptr);

  return true;
}

int ThreadHooker::HookThreadCreate(pthread_t *tidp, const pthread_attr_t *attr,
                                   void *(*start_rtn)(void *), void *arg) {
  if (hookEnabled() && start_rtn != nullptr) {
    auto time = Util::CurrentTimeNs();
    koom::Log::info(thread_tag, "HookThreadCreate");
    auto *hook_arg = new StartRtnArg(arg, Util::CurrentTimeNs(), start_rtn);
    auto *thread_create_arg = hook_arg->thread_create_arg;
    void *thread = koom::CallStack::GetCurrentThread();
    if (thread != nullptr) {
      koom::CallStack::JavaStackTrace(thread,
                                      hook_arg->thread_create_arg->java_stack);
    }
    koom::CallStack::FastUnwind(thread_create_arg->pc,
                                koom::Constant::kMaxCallStackDepth);
    thread_create_arg->stack_time = Util::CurrentTimeNs() - time;
    return pthread_create(tidp, attr,
                          reinterpret_cast<void *(*)(void *)>(HookThreadStart),
                          reinterpret_cast<void *>(hook_arg));
  }
  return pthread_create(tidp, attr, start_rtn, arg);
}

ALWAYS_INLINE void ThreadHooker::HookThreadStart(void *arg) {
  koom::Log::info(thread_tag, "HookThreadStart");
  auto *hookArg = (StartRtnArg *)arg;
  pthread_attr_t attr;
  pthread_t self = pthread_self();
  int state = 0;
  if (pthread_getattr_np(self, &attr) == 0) {
    pthread_attr_getdetachstate(&attr, &state);
  }
  int tid = (int)syscall(SYS_gettid);
  koom::Log::info(thread_tag, "HookThreadStart %p, %d, %d", self, tid,
                  hookArg->thread_create_arg->stack_time);
  auto info = new HookAddInfo(tid, Util::CurrentTimeNs(), self,
                              state == PTHREAD_CREATE_DETACHED,
                              hookArg->thread_create_arg);

  sHookLooper->post(ACTION_ADD_THREAD, info);
  void *(*start_rtn)(void *) = hookArg->start_rtn;
  void *routine_arg = hookArg->arg;
  delete hookArg;
  start_rtn(routine_arg);
}

int ThreadHooker::HookThreadDetach(pthread_t t) {
  if (!hookEnabled()) return pthread_detach(t);

  int c_tid = (int)syscall(SYS_gettid);
  koom::Log::info(thread_tag, "HookThreadDetach c_tid:%0x", c_tid);

  auto info = new HookInfo(t, Util::CurrentTimeNs());
  sHookLooper->post(ACTION_DETACH_THREAD, info);
  return pthread_detach(t);
}

int ThreadHooker::HookThreadJoin(pthread_t t, void **return_value) {
  if (!hookEnabled()) return pthread_join(t, return_value);

  int c_tid = (int)syscall(SYS_gettid);
  koom::Log::info(thread_tag, "HookThreadJoin c_tid:%0x", c_tid);

  auto info = new HookInfo(t, Util::CurrentTimeNs());
  sHookLooper->post(ACTION_JOIN_THREAD, info);
  return pthread_join(t, return_value);
}

void ThreadHooker::HookThreadExit(void *return_value) {
  if (!hookEnabled()) pthread_exit(return_value);

  koom::Log::info(thread_tag, "HookThreadExit");
  int tid = (int)syscall(SYS_gettid);
  char thread_name[16]{};
  prctl(PR_GET_NAME, thread_name);
  auto info =
      new HookExitInfo(pthread_self(), tid, thread_name, Util::CurrentTimeNs());
  sHookLooper->post(ACTION_EXIT_THREAD, info);
  pthread_exit(return_value);
}

void ThreadHooker::Start() { ThreadHooker::InitHook(); }

void ThreadHooker::Stop() {}

bool ThreadHooker::hookEnabled() { return isRunning; }
}  // namespace koom