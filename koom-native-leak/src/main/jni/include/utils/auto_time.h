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

#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_AUTO_TIME_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_AUTO_TIME_H_
#include <log/log.h>

#include <ctime>

class AutoTime {
 public:
  AutoTime(const char *tag = nullptr) : tag_(tag), start_(clock()) {}
  ~AutoTime() {
    clock_t end = clock();
    ALOGI("%s consume time: %f s", tag_ ? tag_ : "",
          (static_cast<double>(end - start_) / CLOCKS_PER_SEC));
  }

 private:
  const char *tag_;
  clock_t start_;
};
#endif  // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_AUTO_TIME_H_
