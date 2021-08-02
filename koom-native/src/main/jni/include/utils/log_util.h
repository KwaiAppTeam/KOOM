/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by lbtrace on 2021.
 *
 */

#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_LOG_UTIL_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_LOG_UTIL_H_

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "leak_monitor"
#endif

#ifndef DLOGI
#ifndef NDEBUG
#define DLOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#else
#define DLOGI(...) ((void)0)
#endif
#endif

#ifndef RLOGE
#define RLOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

#ifdef __clang_analyzer__
// Clang's static analyzer does not see the conditional statement inside
// LogMessage's destructor that will abort on FATAL severity.
#define ABORT_AFTER_LOG_FATAL for (;; abort())

struct LogAbortAfterFullExpr {
  ~LogAbortAfterFullExpr() __attribute__((noreturn)) { abort(); }
  explicit operator bool() const { return false; }
};
// Provides an expression that evaluates to the truthiness of `x`, automatically
// aborting if `c` is true.
#define ABORT_AFTER_LOG_EXPR_IF(c, x) (((c) && ::android::base::LogAbortAfterFullExpr()) || (x))
// Note to the static analyzer that we always execute FATAL logs in practice.
#define MUST_LOG_MESSAGE(severity) (SEVERITY_LAMBDA(severity) == ::android::base::FATAL)
#else
#define ABORT_AFTER_LOG_FATAL
#define ABORT_AFTER_LOG_EXPR_IF(c, x) (x)
#define MUST_LOG_MESSAGE(severity) false
#endif

#define ABORT_AFTER_LOG_FATAL_EXPR(x) ABORT_AFTER_LOG_EXPR_IF(true, x)
#define LIKELY(exp) (__builtin_expect((exp) != 0, true))
#define CHECK(x)                                                                                   \
  (LIKELY((x)) || ABORT_AFTER_LOG_FATAL_EXPR(false) ||                                             \
   RLOGE("%s %d Check failed %s", __FUNCTION__, __LINE__, #x))

#endif // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_LOG_UTIL_H_
