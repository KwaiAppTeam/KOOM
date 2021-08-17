// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#pragma once

#include <sys/cdefs.h>

#include <signal.h>

#include "macros.h"

// Realtime signals reserved for internal use:
//   32 (__SIGRTMIN + 0)        POSIX timers
//   33 (__SIGRTMIN + 1)        libbacktrace
//   34 (__SIGRTMIN + 2)        libcore
//   35 (__SIGRTMIN + 3)        debuggerd
//   36 (__SIGRTMIN + 4)        platform profilers (heapprofd, traced_perf)
//   37 (__SIGRTMIN + 5)        coverage (libprofile-extras)
//   38 (__SIGRTMIN + 6)        heapprofd ART managed heap dumps
//   39 (__SIGRTMIN + 7)        fdtrack
//   40 (__SIGRTMIN + 8)        android_run_on_all_threads (bionic/pthread_internal.cpp)

#define BIONIC_SIGNAL_POSIX_TIMERS (__SIGRTMIN + 0)
#define BIONIC_SIGNAL_BACKTRACE (__SIGRTMIN + 1)
#define BIONIC_SIGNAL_DEBUGGER (__SIGRTMIN + 3)
#define BIONIC_SIGNAL_PROFILER (__SIGRTMIN + 4)
#define BIONIC_SIGNAL_ART_PROFILER (__SIGRTMIN + 6)
#define BIONIC_SIGNAL_FDTRACK (__SIGRTMIN + 7)
#define BIONIC_SIGNAL_RUN_ON_ALL_THREADS (__SIGRTMIN + 8)

extern "C" int sigaddset64(sigset64_t *__set, int __signal) __attribute__((weak));
extern "C" int sigdelset64(sigset64_t *__set, int __signal) __attribute__((weak));

#define __SIGRT_RESERVED 9
static inline __always_inline sigset64_t filter_reserved_signals(sigset64_t sigset, int how) {
  if (!(sigaddset64 && sigdelset64)) {
    abort();
  }
  int (*block)(sigset64_t *, int);
  int (*unblock)(sigset64_t *, int);
  switch (how) {
  case SIG_BLOCK:
    __BIONIC_FALLTHROUGH;
  case SIG_SETMASK:
    block = sigaddset64;
    unblock = sigdelset64;
    break;

  case SIG_UNBLOCK:
    block = sigdelset64;
    unblock = sigaddset64;
    break;
  }

  // The POSIX timer signal must be blocked.
  block(&sigset, __SIGRTMIN + 0);

  // Everything else must remain unblocked.
  unblock(&sigset, __SIGRTMIN + 1);
  unblock(&sigset, __SIGRTMIN + 2);
  unblock(&sigset, __SIGRTMIN + 3);
  unblock(&sigset, __SIGRTMIN + 4);
  unblock(&sigset, __SIGRTMIN + 5);
  unblock(&sigset, __SIGRTMIN + 6);
  unblock(&sigset, __SIGRTMIN + 7);
  unblock(&sigset, __SIGRTMIN + 8);
  return sigset;
}