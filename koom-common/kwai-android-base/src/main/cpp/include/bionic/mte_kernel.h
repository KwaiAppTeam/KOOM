// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#pragma once

// Defines constants used as part of the interface in an experimental MTE branch
// of the Linux kernel, which may be found at:
//
// https://github.com/pcc/linux/tree/android-experimental-mte
//
// This interface should not be considered to be stable.

#ifdef ANDROID_EXPERIMENTAL_MTE

#define HWCAP2_MTE (1 << 18)
#define PROT_MTE 0x20

#define PR_MTE_TCF_SHIFT 1
#define PR_MTE_TCF_NONE (0UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_SYNC (1UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_ASYNC (2UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_MASK (3UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TAG_SHIFT 3
#define PR_MTE_TAG_MASK (0xffffUL << PR_MTE_TAG_SHIFT)

#define SEGV_MTEAERR 8
#define SEGV_MTESERR 9

#define PTRACE_PEEKMTETAGS 33
#define PTRACE_POKEMTETAGS 34

#define NT_ARM_TAGGED_ADDR_CTRL 0x409

#endif