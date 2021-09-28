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
 * Created by shenvsv on 2021.
 *
 */

#ifndef APM_LOG_H
#define APM_LOG_H

#include <android/log.h>
#include <stdio.h>

namespace koom {

class Log {
 public:
  enum Type { Info, Error };

  static void info(const char *tag, const char *format, ...) {
    if (!log_enable) return;
    char log_buffer[kMaxLogLine];
    va_list args;
    va_start(args, format);
    vsnprintf(const_cast<char *>(log_buffer), kMaxLogLine, format, args);
    va_end(args);
    log(Info, tag, log_buffer);
  }

  static void error(const char *tag, const char *format, ...) {
    if (!log_enable) return;
    char log_buffer[kMaxLogLine];
    va_list args;
    va_start(args, format);
    vsnprintf(const_cast<char *>(log_buffer), kMaxLogLine, format, args);
    va_end(args);
    log(Error, tag, log_buffer);
  }

  static bool log_enable;

 private:
  static void log(Type type, const char *tag, char *log_buffer) {
    if (!log_enable) return;
    __android_log_print(type == Info ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        tag, "%s", log_buffer);
  }

  static const int kMaxLogLine = 512;
};
}  // namespace koom

#endif  // APM_LOG_H
