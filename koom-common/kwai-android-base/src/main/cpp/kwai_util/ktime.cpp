/*
 * Copyright (c) 2020. Kwai, Inc. All rights reserved.
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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2020.
 *
 */

#include <cstdint>
#include <ctime>
#include <kwai_util/ktime.h>
#include <kwai_util/kwai_macros.h>

static constexpr uint64_t MILLIS_PER_SEC = 1000;
static constexpr uint64_t MICRO_PER_SEC = 1000 * MILLIS_PER_SEC;
static constexpr uint64_t NANOS_PER_SEC = 1000 * MICRO_PER_SEC;

KWAI_EXPORT uint64_t nanotime() {
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec * NANOS_PER_SEC + ts.tv_nsec);
}

uint64_t now() {
  timespec ts{};
  clock_gettime(CLOCK_REALTIME, &ts);
  return static_cast<uint64_t>(ts.tv_sec * MILLIS_PER_SEC +
                               ts.tv_nsec * MILLIS_PER_SEC / NANOS_PER_SEC);
}