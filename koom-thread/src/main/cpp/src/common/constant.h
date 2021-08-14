//
// Created by lirui on 2020/10/27.
//

#ifndef APM_RESDETECTOR_CONSTANT_H
#define APM_RESDETECTOR_CONSTANT_H

#include <string>
#include <vector>

namespace koom {

namespace Constant {
#define ALWAYS_INLINE __attribute__((always_inline))
const static int max_call_stack_depth = 18;
const static int max_hook_libs = 400;
const static int max_thread_num = 32768;

const static int64_t ns_per_ms = 1000000LL;
const static int64_t ns_per_second = 1000000000LL;

const static int java_callstack_single_gap = 500;//500ms频率限制
const static int java_callstack_loop_count = 6;//循环6次

const static int native_callstack_single_gap = 100;//500ms频率限制
const static int native_callstack_loop_count = 2;//循环6次

const static int dlopen_source_init = 0;

const static int CALL_BACK_TYPE_REPORT = 1;
const static int CALL_BACK_TYPE_CUSTOM_LOG = 2;
}

}

#endif //APM_RESDETECTOR_CONSTANT_H
