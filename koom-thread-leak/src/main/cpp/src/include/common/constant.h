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

#ifndef KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_INCLUDE_COMMON_CONSTANT_H_
#define KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_INCLUDE_COMMON_CONSTANT_H_

#include <string>
#include <vector>

namespace koom {

namespace Constant {
#define ALWAYS_INLINE __attribute__((always_inline))

static const int kMaxCallStackDepth = 18;
static const int kDlopenSourceInit = 0;
}  // namespace Constant
}  // namespace koom
#endif  // KOOM_THREAD_LEAK_SRC_MAIN_CPP_SRC_INCLUDE_COMMON_CONSTANT_H_
