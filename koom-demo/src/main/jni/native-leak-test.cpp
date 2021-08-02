#include <jni.h>
#include <string>
#include <vector>
#include <thread>
#include <deque>
#include <android/log.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#define NOINLINE __attribute__((noinline))
#define LOG_TAG "unreachable"
#define NUM_TEST_CASE 10
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static NOINLINE void TestMallocLeak() {
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 30000001;
            void *mem_ran = malloc(size);
            if (i % 2) {
                LOGI("%s %p size %u", __FUNCTION__, mem_ran, size);
            } else {
                free(mem_ran);
                mem_ran = nullptr;
            }
        }
    }
}

static NOINLINE void TestReallocLeak() {
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 102470;
            void *mem_ran = realloc(nullptr, size);
            if (i % 2) {
                LOGI("%s %p size %u", __FUNCTION__, mem_ran, size);
            } else {
                free(mem_ran);
                mem_ran = nullptr;
            }
        }
    }
}

static NOINLINE void TestCallocLeak() {
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 1023;
            void *mem_ran = calloc(size, 8);
            if (i % 2) {
                LOGI("%s %p size %u", __FUNCTION__, mem_ran, size);
            } else {
                free(mem_ran);
                mem_ran = nullptr;
            }
        }
    }
}

static NOINLINE void TestMemalignLeak() {
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 4095;
            void *mem_ran = memalign(16, size);
            if (i % 2) {
                LOGI("%s %p size %u", __FUNCTION__, mem_ran, size);
            } else {
                free(mem_ran);
                mem_ran = nullptr;
            }
        }
    }
}

static NOINLINE void TestNewLeak() {
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 127;
            std::string *test_string = new std::string("test_string");
            if (i % 2) {
                LOGI("%s %p size %u", __FUNCTION__, test_string, size);
            } else {
                delete test_string;
                test_string = nullptr;
            }
        }
    }
}

static NOINLINE void TestNewArrayLeak() {
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 4095;
            std::string *array = new std::string[size];
            if (i % 2) {
                LOGI("%s %p size %u", __FUNCTION__, array, size);
            } else {
                delete[] array;
                array = nullptr;
            }
        }
    }
}

static NOINLINE void TestContainerLeak() {
    std::vector<std::string> str_vector;
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        str_vector.push_back("Container");
        LOGI("%s %d", __FUNCTION__, __LINE__);
    }
}

static NOINLINE jlong TestJavaRefNative() {
    void **native_object = (void **)malloc(sizeof(void *) * NUM_TEST_CASE);
    LOGI("%s Holder addr %p", __FUNCTION__, native_object);
    for (int i = 0; i < NUM_TEST_CASE; i++) {
        long ran = random();
        if (ran > 0) {
            size_t size = ran % 4095;
            native_object[i] = malloc(size);
            LOGI("%s %p size %u", __FUNCTION__, native_object[i], size);
        }
    }

    return reinterpret_cast<jlong>(native_object);
}

extern "C" JNIEXPORT jlong
Java_com_kwai_koom_demo_nativeleak_NativeLeakTest_triggerLeak(
        JNIEnv* env,
        jclass ,
        jobject unuse/* this */) {
  auto alloc_test = []() -> void {
    TestMallocLeak();
    TestCallocLeak();
    TestReallocLeak();
    TestMemalignLeak();
    TestNewLeak();
    TestNewArrayLeak();
    TestContainerLeak();
  };

//  for (int i = 0; i < 10; i++) {
//    std::thread test_thread(alloc_test);
//    test_thread.detach();
//  }
alloc_test();

  return TestJavaRefNative();
}
