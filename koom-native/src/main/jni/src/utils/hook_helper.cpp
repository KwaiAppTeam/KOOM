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

#include <xhook.h>
#include "utils/hook_helper.h"
#include "utils/log_util.h"

bool HookHelper::HookMethods(std::vector<const std::string> &register_pattern,
    std::vector<const std::string> &ignore_pattern,
    std::vector<std::pair<const std::string, void * const>> &methods) {
  if (register_pattern.empty() || methods.empty()) {
    RLOGE("Hook nothing");
    return false;
  }

#ifndef NDEBUG
  xhook_enable_debug(1);
#endif
  for (auto &pattern : register_pattern) {
      DLOGI("xhook_register pattern %s", pattern.c_str());
      for (auto &method : methods) {
      if (xhook_register(pattern.c_str(), method.first.c_str(),
                     method.second, nullptr) != EXIT_SUCCESS) {
        RLOGE("xhook_register fail");
        return false;
      }
    }
  }

  for (auto &pattern : ignore_pattern) {
      DLOGI("xhook_register ignore pattern %s", pattern.c_str());
      for (auto &method : methods) {
      if (xhook_ignore(pattern.c_str(), method.first.c_str()) != EXIT_SUCCESS) {
        RLOGE("xhook_ignore fail");
        return false;
      }
    }
  }

  return true;
}

bool HookHelper::SyncRefreshHook() {
  return xhook_refresh(0) == EXIT_SUCCESS;
}

bool HookHelper::AsyncRefreshHook() {
  return xhook_refresh(1) == EXIT_SUCCESS;
}