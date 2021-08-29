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

#ifndef APM_RESOURCEDATA_H
#define APM_RESOURCEDATA_H

#include <map>

#include "common/callstack.h"
#include "common/log.h"
#include "common/util.h"
#include "loop_item.h"
#include "rapidjson/writer.h"
#include "thread_item.h"

namespace koom {

class ThreadHolder {
 public:
  void AddThread(int tid, pthread_t pthread, bool isThreadDetached,
                 int64_t start_time, ThreadCreateArg* create_arg);
  void JoinThread(pthread_t threadId);
  void ExitThread(pthread_t threadId, std::string& threadName, long long int i);
  void DetachThread(pthread_t threadId);
  void ReportThreadLeak(long long time);

 private:
  std::map<pthread_t, ThreadItem> leakThreadMap;
  std::map<pthread_t, ThreadItem> threadMap;
  void WriteThreadJson(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                       ThreadItem& thread_item);
  void Clear() {
    leakThreadMap.clear();
    threadMap.clear();
  }
};
}  // namespace koom
#endif  // APM_RESOURCEDATA_H
