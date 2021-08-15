#ifndef APM_THREAD_HOOK_H
#define APM_THREAD_HOOK_H

#include <sys/types.h>
#include "common/callstack.h"
#include <pthread.h>
#include <jni.h>
#include <android/dlext.h>
#include "hook_looper.h"
#include "koom.h"

namespace koom {

class ThreadHooker {
 public:
  static void Stop();
  static void Start();

 private:
  static void HookThreadStart(void *arg);
  static int HookThreadCreate(pthread_t *tidp, const pthread_attr_t *attr,
                              void *(*start_rtn)(void *), void *arg);
  static int HookThreadJoin(pthread_t t, void **return_value);
  static int HookThreadDetach(pthread_t t);
  static void HookThreadExit(void *return_value);
  static bool RegisterSo(const std::string &lib, int source);
  static void InitHook();
  static void DlopenCallback(std::set<std::string> &libs, int source, std::string &sourcelib);
  static void HookLibs(std::set<std::string> &libs, int source);
  static bool hookEnabled();
};

class StartRtnArg {
 public:
  void *arg;
  long long time;
  long long stack_time;
  void *(*start_rtn)(void *);
  std::string callstack{};
  uintptr_t pc[koom::Constant::max_call_stack_depth]{};

  StartRtnArg(void *arg,
              long long time,
              void *(*start_rtn)(void *)) {
    this->arg = arg;
    this->time = time;
    this->start_rtn = start_rtn;
  }
};
}

#endif //APM_THREAD_HOOK_H
