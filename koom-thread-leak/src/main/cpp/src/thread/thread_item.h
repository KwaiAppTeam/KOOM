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

#ifndef APM_THREAD_H
#define APM_THREAD_H
#include <string>
namespace koom {

class ThreadItem {
 public:
  int id{};
  int64_t create_time{};
  std::string create_call_stack;
  std::string collect_mode{};
  bool thread_detached{};
  long long startTime{};
  long long exitTime{};
  bool thread_reported{};
  pthread_t thread_internal_id{};
  std::string name{};

  ThreadItem();
  ThreadItem(const ThreadItem &threadItem);
  void Clear();
};

#endif  // APM_THREAD_H
}