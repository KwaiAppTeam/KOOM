#ifndef APM_CALLSTACK_H
#define APM_CALLSTACK_H

#include <ostream>
#include <sstream>
#include "util.h"
#include "constant.h"
#include <unistd.h>
#include <dlfcn.h>
#include <kwai_linker/kwai_dlfcn.h>
//#include "tls_defines.h"
#include <fast_unwind/fast_unwind.h>
#include <unwindstack/Unwinder.h>

namespace koom {

// libart.so
using dump_java_stack_above_o_ptr = void (*)(void *, std::ostream &os, bool, bool);
using dump_java_stack_ptr = void (*)(void *, std::ostream &os);

class CallStack {

  enum Type {
    java, native
  };

 private:
  static dump_java_stack_above_o_ptr dump_java_stack_above_o;
  static dump_java_stack_ptr dump_java_stack;
  static pthread_key_t pthread_key_self;
  static unwindstack::UnwinderFromPid *unwinder;
  static std::atomic<bool> inSymbolize;

  static std::atomic<bool> dumpingJava;
  static std::atomic<bool> dumpingNative;
  static std::atomic<bool> disableJava;
  static std::atomic<bool> disableNative;

  static timespec lastJavaTime;
  static timespec lastNativeTime;
  static int java_gap_count;
  static int native_gap_count;

 public:
  static int java_single_gap;
  static int native_single_gap;
  static int max_java_loop_count;
  static int max_native_loop_count;

  static void Init();

  static void SetJava(int gap, int loop);

  static void DisableJava();

  static void DisableNative();

  static void SetNative(int gap, int loop);

  static void JavaStackTrace(void *thread, std::ostream &os);

  static size_t FastUnwind(uintptr_t *buf, size_t num_entries);

  static std::string SymbolizePc(uintptr_t pc, int index);

  static void *GetCurrentThread();

  static int64_t GetCurrentTimeLimit(Type type);

  static bool MeetFrequencyLimit(Type type);

};

}

#endif //APM_CALLSTACK_H
