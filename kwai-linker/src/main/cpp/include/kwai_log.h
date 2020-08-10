// Copyright 2020 Kwai, Inc. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//         http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//         limitations under the License.

// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#ifndef KWAI_LOG_H
#define KWAI_LOG_H

#include <android/log.h>
#include <errno.h>

#ifndef LOG_TAG
#define LOG_TAG "kwai"
#endif

#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) __android_log_print(priority, tag, __VA_ARGS__)
#endif // LOG_PRI

#ifndef ALOG
#define ALOG(priority, tag, ...) LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif // ALOG

#ifndef ALOGD
#if NDK_DEBUG
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
#define ALOGD(...) ((void)0)
#endif // NDK_DEBUG
#endif // ALOGD

#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGF
#define ALOGF(...) ((void)ALOG(LOG_FATAL, LOG_TAG, __VA_ARGS__))
#endif

#ifndef CHECK
#define CHECK(assertion)                                                                           \
  if (!(assertion)) {                                                                              \
    ALOGE("CHECK failed at %s (line: %d) - <%s>: %s: %s", __FILE__, __LINE__, __FUNCTION__,        \
          #assertion, strerror(errno));                                                            \
  }
#endif

#ifndef CHECKV
#define CHECKV(assertion)                                                                          \
  if (!(assertion)) {                                                                              \
    ALOGE("CHECK failed at %s (line: %d) - <%s>: %s: %s", __FILE__, __LINE__, __FUNCTION__,        \
          #assertion, strerror(errno));                                                            \
    return;                                                                                        \
  }
#endif

#ifndef CHECKI
#define CHECKI(assertion)                                                                          \
  if (!(assertion)) {                                                                              \
    ALOGE("CHECK failed at %s (line: %d) - <%s>: %s: %s", __FILE__, __LINE__, __FUNCTION__,        \
          #assertion, strerror(errno));                                                            \
    return -1;                                                                                     \
  }
#endif

#ifndef CHECKP
#define CHECKP(assertion)                                                                          \
  if (!(assertion)) {                                                                              \
    ALOGE("CHECK failed at %s (line: %d) - <%s>: %s: %s", __FILE__, __LINE__, __FUNCTION__,        \
          #assertion, strerror(errno));                                                            \
    return nullptr;                                                                                \
  }
#endif

#ifndef CHECKB
#define CHECKB(assertion)                                                                          \
  if (!(assertion)) {                                                                              \
    ALOGE("CHECK failed at %s (line: %d) - <%s>: %s: %s", __FILE__, __LINE__, __FUNCTION__,        \
          #assertion, strerror(errno));                                                            \
    return false;                                                                                  \
  }
#endif

#ifndef FINISHP_FUC
#define FINISHP_FUC(assertion, func)                                                               \
  if (!(assertion)) {                                                                              \
    ALOGE("CHECK failed at %s (line: %d) - <%s>: %s: %s", __FILE__, __LINE__, __FUNCTION__,        \
          #assertion, strerror(errno));                                                            \
    func();                                                                                        \
    return nullptr;                                                                                \
  }
#endif

#endif // KWAI_LOG_H
