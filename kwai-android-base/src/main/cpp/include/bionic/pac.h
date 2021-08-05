// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#pragma once

#include <stddef.h>

inline uintptr_t __bionic_clear_pac_bits(uintptr_t ptr) {
#if defined(__aarch64__)
  register uintptr_t x30 __asm("x30") = ptr;
  // This is a NOP on pre-Armv8.3-A architectures.
  asm("xpaclri" : "+r"(x30));
  return x30;
#else
  return ptr;
#endif
}