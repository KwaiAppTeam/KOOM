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

#ifndef APM_UTIL_H
#define APM_UTIL_H

#include <dirent.h>
#include <jni.h>

#include <fstream>
#include <map>
#include <set>
#include <streambuf>
#include <string>
#include <vector>

#include "log.h"

namespace koom {

class Util {
 public:
  static int android_api;

  static void Init() { android_api = android_get_device_api_level(); }

  static int AndroidApi() { return android_api; }

  static timespec CurrentClockTime() {
    struct timespec now_time {};
    clock_gettime(CLOCK_MONOTONIC, &now_time);
    return now_time;
  }

  static long long CurrentTimeNs() {
    struct timespec now_time {};
    clock_gettime(CLOCK_MONOTONIC, &now_time);
    return now_time.tv_sec * 1000000000LL + now_time.tv_nsec;
  }

  static std::vector<std::string> Split(const std::string &s, char seperator) {
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find(seperator, pos)) != std::string::npos) {
      std::string substring(s.substr(prev_pos, pos - prev_pos));
      output.push_back(substring);
      prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos));  // Last word

    return output;
  }
};
}  // namespace koom
#endif  // APM_UTIL_H
