#ifndef APM_CALLSTACK_H
#define APM_CALLSTACK_H

#include <ostream>
#include <sstream>
#include "util.h"
#include "constant.h"
#include <unistd.h>
#include <fast_unwind/fast_unwind.h>
#include <unwindstack/Unwinder.h>

namespace koom {

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

  static std::atomic<bool> disableJava;
  static std::atomic<bool> disableNative;

  static std::mutex dumpJavaLock;

 public:
  static void Init();

  static void DisableJava();

  static void DisableNative();

  static void JavaStackTrace(void *thread, std::ostream &os);

  static size_t FastUnwind(uintptr_t *buf, size_t num_entries);

  static std::string SymbolizePc(uintptr_t pc, int index);

  static void *GetCurrentThread();
};

}

#endif //APM_CALLSTACK_H
