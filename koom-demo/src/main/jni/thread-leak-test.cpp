#include <jni.h>
#include <string>
#include <vector>
#include <thread>
#include <android/log.h>
#include <jni.h>

#define NOINLINE __attribute__((noinline))
#define LOG_TAG "ThreadLeakTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static NOINLINE void TestThreadLeak(int64_t delay) {
  std::thread test_thread([](int64_t delay) {
    pthread_setname_np(pthread_self(), "test_thread");
    LOGI("test_thread run");
    std::thread *test_thread_1;
    std::thread *test_thread_2;
    test_thread_1 = new std::thread([]() {
      pthread_setname_np(pthread_self(), "test_thread_1");
      LOGI("test_thread_1 run");
    });
    test_thread_2 = new std::thread([]() {
      pthread_setname_np(pthread_self(), "test_thread_2");
      LOGI("test_thread_2 run");
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    test_thread_1->detach();
    LOGI("test_thread_1 detach");
    test_thread_2->join();
    LOGI("test_thread_2 join");
  }, delay);
  test_thread.detach();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kwai_koom_demo_threadleak_ThreadLeakTest_triggerLeak(
    JNIEnv *env,
    jclass, jlong delay) {
  TestThreadLeak(delay);
}