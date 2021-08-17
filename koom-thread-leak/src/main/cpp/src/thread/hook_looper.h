//
// Created by shenvsv on 1/14/21.
//

#ifndef APM_KOOM_THREAD_SRC_MAIN_CPP_SRC_THREAD_HOOK_LOOPER_H_
#define APM_KOOM_THREAD_SRC_MAIN_CPP_SRC_THREAD_HOOK_LOOPER_H_

#include <jni.h>
#include <map>
#include "common/looper.h"
#include "thread_holder.h"
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
struct SimpleHookInfo {
  long long time;
  SimpleHookInfo(long long time) {
    this->time = time;
  }
};
struct HookCollectStartInfo {
  std::string mode;
  HookCollectStartInfo(const char *mode) {
    this->mode = std::string(mode);
  }
};
struct HookInfo {
  pthread_t threadId;
  long long time;
  HookInfo(pthread_t threadId, long long time) {
    this->threadId = threadId;
    this->time = time;
  }
};
struct HookExitInfo {
  pthread_t threadId;
  long long time;
  int tid;
  std::string threadName;
  HookExitInfo(pthread_t threadId, int tid, char *threadName, long long time) {
    this->threadId = threadId;
    this->tid = tid;
    this->threadName.assign(threadName);
    this->time = time;
  }
};
class HookAddInfo {
 public:
  int tid;
  long long time;
  pthread_t pthread;
  bool isThreadDetached;
  int64_t allocateTime;
  std::string java_call_stack;
  uintptr_t pc[koom::Constant::max_call_stack_depth]{};

  HookAddInfo(int tid,
              long long time,
              pthread_t pthread,
              bool isThreadDetached,
              int64_t allocateTime,
              std::string &java_call_stack,
              uintptr_t (&pc)[koom::Constant::max_call_stack_depth]) {
    this->tid = tid;
    this->time = time;
    this->pthread = pthread;
    this->isThreadDetached = isThreadDetached;
    this->allocateTime = allocateTime;
    this->java_call_stack.assign(java_call_stack);
    memcpy(this->pc, pc, sizeof(this->pc));
  };
};
class HookLooper : public looper {
 public:
  koom::ThreadHolder *holder;
  HookLooper();
  ~HookLooper();
  void handle(int what, void *data);
  void post(int what, void *data);
};
#endif //APM_KOOM_THREAD_SRC_MAIN_CPP_SRC_THREAD_HOOK_LOOPER_H_