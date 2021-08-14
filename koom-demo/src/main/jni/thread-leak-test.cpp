#include <jni.h>
#include <string>
#include <vector>
#include <thread>
#include <android/log.h>
#include <jni.h>

#define NOINLINE __attribute__((noinline))
#define LOG_TAG "ThreadLeakTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static NOINLINE void TestThreadLeak() {
  static std::thread *test_thread_1;
  static std::thread *test_thread_2;
  static std::thread *test_thread_3;
  static std::thread *test_thread_4;
  test_thread_1 = new std::thread([]() {
    pthread_setname_np(pthread_self(), "test_thread_1");
    LOGI("test_thread_1 run");
  });

  test_thread_2 = new std::thread([]() {
    pthread_setname_np(pthread_self(), "test_thread_2");
    LOGI("test_thread_2 run");
  });

  test_thread_3 = new std::thread([]() {
    pthread_setname_np(pthread_self(), "test_thread_3");
    LOGI("test_thread_3 run");
  });

  test_thread_4 = new std::thread([]() {
    pthread_setname_np(pthread_self(), "test_thread_4");
    LOGI("test_thread_4 run");
  });

  std::thread detach_thread([]() {
    pthread_setname_np(pthread_self(), "detach_thread");
    LOGI("detach_thread run");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    test_thread_1->detach();
    LOGI("test_thread_1 detach");
    test_thread_2->join();
    LOGI("test_thread_2 join");
    delete test_thread_1;
    delete test_thread_2;

    std::this_thread::sleep_for(std::chrono::seconds(10));
    test_thread_3->detach();
    LOGI("test_thread_3 detach");
    test_thread_4->join();
    LOGI("test_thread_4 join");
    delete test_thread_3;
    delete test_thread_4;
  });
  detach_thread.detach();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kwai_koom_demo_threadleak_ThreadLeakTest_triggerLeak(
    JNIEnv *env,
    jclass) {
  TestThreadLeak();
}