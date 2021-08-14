#include "thread_hook.h"
#include <dlopencb/dlopencb.h>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <xhook.h>
#include <syscall.h>

namespace koom {

const char *thread_tag = "koom-thread";

const char *ignore_libs[] = {};

char *ignore_libs_extra[] = {};

bool IsBuiltInLibsIgnore(const std::string &lib) {
  for (const auto &ignoreLib : ignore_libs) {
    if (lib.find(ignoreLib) != -1) {
      return true;
    }
  }
  return false;
}

bool IsExtraLibsIgnore(const std::string &lib) {
  for (const auto &ignoreLib : ignore_libs_extra) {
    if (lib.find(ignoreLib) != -1) {
      return true;
    }
  }
  return false;
}

static bool IsLibIgnored(const std::string &lib) {
  return IsBuiltInLibsIgnore(lib) || IsExtraLibsIgnore(lib);
}

int Callback(struct dl_phdr_info *info, size_t size, void *data) {
  auto *libs = static_cast<std::set<std::string> *>(data);
  libs->insert(info->dlpi_name);
  return 0;
}

void ThreadHooker::InitHook() {
  koom::Log::info(thread_tag, "HookSo init hook");
  std::set<std::string> libs;
//  dlopencb::setDebug(true);
  DlopenCb::GetInstance().GetLoadedLibs(libs);
  HookLibs(libs, Constant::dlopen_source_init);
  DlopenCb::GetInstance().AddCallback(DlopenCallback);
}

void ThreadHooker::DlopenCallback(std::set<std::string> &libs, int source, std::string
&source_lib) {
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
  xhook_register(lib_ctr, "pthread_create", reinterpret_cast<void *>(HookThreadCreate), nullptr);
  xhook_register(lib_ctr, "pthread_detach", reinterpret_cast<void *>(HookThreadDetach), nullptr);
  xhook_register(lib_ctr, "pthread_join", reinterpret_cast<void *>(HookThreadJoin), nullptr);
  xhook_register(lib_ctr, "pthread_exit", reinterpret_cast<void *>(HookThreadExit), nullptr);

  return true;
}

int ThreadHooker::HookThreadCreate(pthread_t *tidp,
                                   const pthread_attr_t *attr,
                                   void *(*start_rtn)(
                                          void *),
                                   void *arg) {
  if (hookEnabled() && start_rtn != nullptr) {
    auto time = Util::CurrentTimeNs();
    koom::Log::info(thread_tag,
                    "HookThreadCreate");
    auto *hookArg = new StartRtnArg(arg, Util::CurrentTimeNs(), start_rtn);
    void *thread = koom::CallStack::GetCurrentThread();
    if (thread != nullptr) {
      std::ostringstream stack_stream;
      koom::CallStack::JavaStackTrace(thread, stack_stream);
      hookArg->callstack = stack_stream.str();
    }
    size_t num_frames = 0;
    if (thread == nullptr || hookArg->callstack.empty() || hookArg->callstack.find("no") !=
        std::string::npos) {
      num_frames = koom::CallStack::FastUnwind(hookArg->pc, koom::Constant::max_call_stack_depth);
    }
    hookArg->stack_time = Util::CurrentTimeNs() - time;
    return pthread_create(tidp,
                          attr,
                          reinterpret_cast<void *(*)(void *)>(HookThreadStart),
                          reinterpret_cast<void *>(hookArg));
  }
  return pthread_create(tidp, attr, start_rtn, arg);
}

ALWAYS_INLINE void ThreadHooker::HookThreadStart(void *arg) {
  koom::Log::info(thread_tag, "HookThreadStart");
  auto *hookArg = (StartRtnArg *) arg;
  pthread_attr_t attr;
  pthread_t self = pthread_self();
  int state = 0;
  if (pthread_getattr_np(self, &attr) == 0) {
    pthread_attr_getdetachstate(&attr, &state);
  }
  int tid = (int) syscall(SYS_gettid);
  koom::Log::info(thread_tag, "HookThreadStart %p, %d, %d", self, tid, hookArg->stack_time);
  auto info = new HookAddInfo(tid, Util::CurrentTimeNs(), self,
                              state == PTHREAD_CREATE_DETACHED,
                              hookArg->time,
                              hookArg->callstack,
                              hookArg->pc);

  sHookLooper->post(ACTION_ADD_THREAD, info);
  void *(*start_rtn)(void *) = hookArg->start_rtn;
  void *routine_arg = hookArg->arg;
  delete hookArg;
  start_rtn(routine_arg);
}

int ThreadHooker::HookThreadDetach(pthread_t t) {
  if (!hookEnabled()) return pthread_detach(t);

  int c_tid = (int) syscall(SYS_gettid);
  koom::Log::info(thread_tag, "HookThreadDetach c_tid:%0x",
                  c_tid);

  auto info = new HookInfo(t, Util::CurrentTimeNs());
  sHookLooper->post(ACTION_DETACH_THREAD, info);
  return pthread_detach(t);
}

int ThreadHooker::HookThreadJoin(pthread_t t,
                                 void **return_value) {
  if (!hookEnabled()) return pthread_join(t, return_value);

  int c_tid = (int) syscall(SYS_gettid);
  koom::Log::info(thread_tag, "HookThreadJoin c_tid:%0x",
                  c_tid);

  auto info = new HookInfo(t, Util::CurrentTimeNs());
  sHookLooper->post(ACTION_JOIN_THREAD, info);
  return pthread_join(t, return_value);
}

void ThreadHooker::HookThreadExit(void *return_value) {
  if (!hookEnabled()) pthread_exit(return_value);

  koom::Log::info(thread_tag, "HookThreadExit");
  int tid = (int) syscall(SYS_gettid);
  char thread_name[16]{};
  prctl(PR_GET_NAME, thread_name);
  auto info = new HookExitInfo(pthread_self(), tid, thread_name, Util::CurrentTimeNs());
  sHookLooper->post(ACTION_EXIT_THREAD, info);
  pthread_exit(return_value);
}

void ThreadHooker::Start() {
  ThreadHooker::InitHook();
}

void ThreadHooker::Stop() {
}

bool ThreadHooker::hookEnabled() {
  return isRunning;
}
}