/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <optional>

#include <android/log.h>

namespace android {
namespace base {

struct LibLogFunctions {
  void (*__android_log_set_logger)(__android_logger_function logger);
  void (*__android_log_write_log_message)(struct __android_log_message *log_message);

  void (*__android_log_logd_logger)(const struct __android_log_message *log_message);
  void (*__android_log_stderr_logger)(const struct __android_log_message *log_message);

  void (*__android_log_set_aborter)(__android_aborter_function aborter);
  void (*__android_log_call_aborter)(const char *abort_message);
  void (*__android_log_default_aborter)(const char *abort_message);
  int32_t (*__android_log_set_minimum_priority)(int32_t priority);
  int32_t (*__android_log_get_minimum_priority)();
  void (*__android_log_set_default_tag)(const char *tag);
};

const std::optional<LibLogFunctions> &GetLibLogFunctions();

} // namespace base
} // namespace android
