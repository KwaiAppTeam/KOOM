/*
 * Copyright (c) 2020. Kwai, Inc. All rights reserved.
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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2020.
 *
 */

#pragma once

#include <android/log.h>
#include <async_safe/log.h>
#include <errno.h>
#include <log/log.h>

#ifndef LOG_TAG
#define LOG_TAG "unknown"
#endif

#ifndef KLOGE
#define KLOGE(assertion)                                                                           \
  async_safe_format_log(ANDROID_LOG_ERROR, LOG_TAG,                                                \
                        "CHECK failed at %s (line: %d) - <%s>: "                                   \
                        "%s: %s",                                                                  \
                        __FILE__, __LINE__, __FUNCTION__, #assertion, strerror(errno));
#endif

#ifndef KCHECK
#define KCHECK(assertion)                                                                          \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
  }
#endif

#ifndef KCHECKV
#define KCHECKV(assertion)                                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    return;                                                                                        \
  }
#endif

#ifndef KCHECKI
#define KCHECKI(assertion)                                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    return -1;                                                                                     \
  }
#endif

#ifndef KCHECKP
#define KCHECKP(assertion)                                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    return nullptr;                                                                                \
  }
#endif

#ifndef KCHECKB
#define KCHECKB(assertion)                                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    return false;                                                                                  \
  }
#endif

#ifndef KFINISHI_FNC
#define KFINISHI_FNC(assertion, func, ...)                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    func(__VA_ARGS__);                                                                             \
    return -1;                                                                                     \
  }
#endif

#ifndef KFINISHP_FNC
#define KFINISHP_FNC(assertion, func, ...)                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    func(__VA_ARGS__);                                                                             \
    return nullptr;                                                                                \
  }
#endif

#ifndef KFINISHV_FNC
#define KFINISHV_FNC(assertion, func, ...)                                                         \
  if (!(assertion)) {                                                                              \
    KLOGE(assertion)                                                                               \
    func(__VA_ARGS__);                                                                             \
    return;                                                                                        \
  }
#endif