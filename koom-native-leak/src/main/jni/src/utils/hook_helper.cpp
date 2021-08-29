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

#define LOG_TAG "hook_helper"
#include "utils/hook_helper.h"

#include <dlopencb.h>
#include <log/log.h>
#include <xhook.h>

std::vector<const std::string> HookHelper::register_pattern_;
std::vector<const std::string> HookHelper::ignore_pattern_;
std::vector<std::pair<const std::string, void *const>> HookHelper::methods_;

bool HookHelper::HookMethods(
    std::vector<const std::string> &register_pattern,
    std::vector<const std::string> &ignore_pattern,
    std::vector<std::pair<const std::string, void *const>> &methods) {
  if (register_pattern.empty() || methods.empty()) {
    ALOGE("Hook nothing");
    return false;
  }

  register_pattern_ = std::move(register_pattern);
  ignore_pattern_ = std::move(ignore_pattern);
  methods_ = std::move(methods);
  DlopenCb::GetInstance().AddCallback(Callback);
  return HookImpl();
}

void HookHelper::UnHookMethods() {
  DlopenCb::GetInstance().RemoveCallback(Callback);
  register_pattern_.clear();
  ignore_pattern_.clear();
  methods_.clear();
}

void HookHelper::Callback(std::set<std::string> &, int, std::string &) {
  HookImpl();
}

bool HookHelper::HookImpl() {
  pthread_mutex_lock(&DlopenCb::hook_mutex);
  xhook_clear();
  for (auto &pattern : register_pattern_) {
    for (auto &method : methods_) {
      if (xhook_register(pattern.c_str(), method.first.c_str(), method.second,
                         nullptr) != EXIT_SUCCESS) {
        ALOGE("xhook_register pattern %s method %s fail", pattern.c_str(),
              method.first.c_str());
        pthread_mutex_unlock(&DlopenCb::hook_mutex);
        return false;
      }
    }
  }

  for (auto &pattern : ignore_pattern_) {
    for (auto &method : methods_) {
      if (xhook_ignore(pattern.c_str(), method.first.c_str()) != EXIT_SUCCESS) {
        ALOGE("xhook_ignore pattern %s method %s fail", pattern.c_str(),
              method.first.c_str());
        pthread_mutex_unlock(&DlopenCb::hook_mutex);
        return false;
      }
    }
  }

  int ret = xhook_refresh(0);
  pthread_mutex_unlock(&DlopenCb::hook_mutex);
  return ret == 0;
}