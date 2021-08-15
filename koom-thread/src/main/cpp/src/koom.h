#ifndef APM_KOOM_H
#define APM_KOOM_H

#include <jni.h>
#include "thread/hook_looper.h"

namespace koom {

extern JavaVM *java_vm_;

extern jclass nativeHandlerClass;

extern jmethodID javaCallbackMethod;

extern HookLooper *sHookLooper;

extern std::atomic<bool> isRunning;

extern long threadLeakDelay;

extern void Init(JavaVM *vm, JNIEnv *p_env);

extern void Start();

extern void Stop();

extern void Report(JNIEnv *env,
                   jobject obj,
                   jstring type);

extern void Refresh();

JNIEnv *GetEnv(bool doAttach = true);

void JavaCallback(int type, const char *key, const char *value, bool doAttach = true);
}

#endif //APM_KOOM_H
