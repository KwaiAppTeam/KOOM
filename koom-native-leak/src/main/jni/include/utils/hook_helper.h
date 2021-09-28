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

#ifndef KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_HOOK_HELPER_H_
#define KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_HOOK_HELPER_H_

#include <mutex>
#include <set>
#include <string>
#include <vector>

class HookHelper {
 public:
  static bool HookMethods(
      std::vector<const std::string> &register_pattern,
      std::vector<const std::string> &ignore_pattern,
      std::vector<std::pair<const std::string, void *const>> &methods);
  static void UnHookMethods();

 private:
  static void Callback(std::set<std::string> &, int, std::string &);
  static bool HookImpl();
  static std::vector<const std::string> register_pattern_;
  static std::vector<const std::string> ignore_pattern_;
  static std::vector<std::pair<const std::string, void *const>> methods_;
};
#endif  // KOOM_NATIVE_OOM_SRC_MAIN_JNI_INCLUDE_UTILS_HOOK_HELPER_H_
