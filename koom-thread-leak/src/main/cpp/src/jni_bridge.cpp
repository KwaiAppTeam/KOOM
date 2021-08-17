#include <jni.h>
#include "koom.h"
#include "common/callstack.h"

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  koom::Log::info("koom-thread", "JNI_OnLoad");
  JNIEnv *env = nullptr;
  if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }
  koom::Init(vm, env);
  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_disableJavaStack(
    JNIEnv *env, jobject jObject) {
  koom::CallStack::DisableJava();
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_disableNativeStack(
    JNIEnv *env, jobject jObject) {
  koom::CallStack::DisableNative();
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_start(JNIEnv *env, jobject obj) {
  koom::Log::info("koom-thread", "start");
  koom::Start();
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_refresh(JNIEnv *env, jobject obj) {
  koom::Refresh();
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_stop(JNIEnv *env, jobject obj) {
  koom::Stop();
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_setThreadLeakDelay(JNIEnv *env, jobject
thiz, jlong delay) {
  koom::threadLeakDelay = delay;
}

JNIEXPORT void JNICALL Java_com_kwai_performance_overhead_thread_monitor_NativeHandler_enableNativeLog(
    JNIEnv *env,
    jobject
    jObject) {
  koom::Log::logEnable = true;
}
}


