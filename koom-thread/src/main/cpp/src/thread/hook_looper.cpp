//
// Created by shenvsv on 1/14/21.
//

#include "hook_looper.h"
#include "koom.h"
const char *looper_tag = "koom-hook-looper";
HookLooper::HookLooper() : looper() {
  this->holder = new koom::ThreadHolder();
}
HookLooper::~HookLooper() {
  delete this->holder;
}
void HookLooper::handle(int what, void *data) {
  looper::handle(what, data);
  switch (what) {
    case ACTION_ADD_THREAD: {
      koom::Log::info(looper_tag, "AddThread");
      auto info = static_cast<HookAddInfo *>(data);
      holder->AddThread(info->tid, info->pthread, info->isThreadDetached,
                        info->allocateTime, info->time, info->java_call_stack, info->pc);
      delete info;
      break;
    }
    case ACTION_JOIN_THREAD: {
      koom::Log::info(looper_tag, "JoinThread");
      auto info = static_cast<HookInfo *>(data);
      holder->JoinThread(info->threadId);
      delete info;
      break;
    }
    case ACTION_DETACH_THREAD: {
      koom::Log::info(looper_tag, "DetachThread");
      auto info = static_cast<HookInfo *>(data);
      holder->DetachThread(info->threadId);
      delete info;
      break;
    }
    case ACTION_EXIT_THREAD: {
      koom::Log::info(looper_tag, "ExitThread");
      auto info = static_cast<HookExitInfo *>(data);
      holder->ExitThread(info->threadId, info->threadName, info->time);
      delete info;
      break;
    }
    case ACTION_REFRESH: {
      koom::Log::info(looper_tag, "Refresh");
      auto info = static_cast<SimpleHookInfo *>(data);
      holder->ReportThreadLeak(info->time);
      delete info;
      break;
    }
    default: {

    }
  }
}
void HookLooper::post(int what, void *data) {
  looper::post(what, data);
}
