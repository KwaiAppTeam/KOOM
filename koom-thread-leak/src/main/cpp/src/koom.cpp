//
// Created by lirui on 2020/11/3.
//

#include <jni.h>
#include "koom.h"
#include "common/util.h"
#include "common/callstack.h"
#include "thread/thread_hook.h"

namespace koom {

int Util::android_api;
bool Log::logEnable = false;

JavaVM *java_vm_;
jclass nativeHandlerClass;
jmethodID javaCallbackMethod;
HookLooper *sHookLooper;
std::atomic<bool> isRunning;

long threadLeakDelay;

void Init(JavaVM *vm, _JNIEnv *env) {
  java_vm_ = vm;
  auto clazz = env->FindClass("com/kwai/performance/overhead/thread/monitor/NativeHandler");
  nativeHandlerClass = static_cast<jclass>(env->NewGlobalRef(clazz));
  javaCallbackMethod = env->GetStaticMethodID(nativeHandlerClass,
                                              "nativeCallback",
                                              "(ILjava/lang/String;Ljava/lang/String;)V");
  Util::Init();
  Log::info("koom", "Init, android api:%d", Util::AndroidApi());
  CallStack::Init();
  sHookLooper = new HookLooper();
}

void Start() {
  if (isRunning) {
    return;
  }
  // 初始化数据
  sHookLooper->post(ACTION_INIT, nullptr);
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
  int status = java_vm_->GetEnv((void **) &env, JNI_VERSION_1_6);
  if ((status == JNI_EDETACHED || env == nullptr) && doAttach) {
    status = java_vm_->AttachCurrentThread(&env, nullptr);
    if (status < 0) {
      env = nullptr;
    }
  }
  return env;
}

void JavaCallback(int type, const char *key, const char *value, bool doAttach) {
  JNIEnv *env = GetEnv(doAttach);
  if (env != nullptr && key != nullptr && value != nullptr) {
    Log::error("koom", "JavaCallback %d", strlen(value));
    jstring string_key = env->NewStringUTF(key);
    jstring string_value = env->NewStringUTF(value);
    env->CallStaticVoidMethod(nativeHandlerClass, javaCallbackMethod, type, string_key,
                              string_value);
    Log::info("koom", "JavaCallback finished");
  } else {
    Log::info("koom", "JavaCallback fail no JNIEnv");
  }
}

}