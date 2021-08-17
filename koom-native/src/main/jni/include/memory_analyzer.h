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

#ifndef KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_ANALYZER_H_
#define KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_ANALYZER_H_

#include <string>
#include <vector>

namespace kwai {
namespace leak_monitor {
class MemoryAnalyzer {
 public:
  MemoryAnalyzer();
  ~MemoryAnalyzer();
  bool IsValid();
  std::vector<std::pair<uintptr_t, size_t>> CollectUnreachableMem();

 private:
  using GetUnreachableFn = std::string (*)(bool, size_t);
  GetUnreachableFn get_unreachable_fn_;
  void *handle_;
};
}  // namespace leak_monitor
}  // namespace kwai
#endif  // KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_ANALYZER_H_
