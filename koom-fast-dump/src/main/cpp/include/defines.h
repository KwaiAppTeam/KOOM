/*
 * Copyright (c) 2025. Kwai, Inc. All rights reserved.
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
 * Created by wangzefeng <wangzefeng@kuaishou.com> on 2025.
 *
 */

#ifndef KOOM_DEFINES_H_
#define KOOM_DEFINES_H_

#define __ANDROID_API_V__ 35

namespace kwai {
namespace leak_monitor {

// What caused the GC?
enum GcCause {
  // Not a real GC cause, used to prevent hprof running in the middle of GC.
  kGcCauseHprof = 15,
};

// Which types of collections are able to be performed.
enum CollectorType {
  // Hprof fake collector.
  // NOTE: Use any fake collector is ok
  //  on AOSP Android 15, kCollectorTypeHprof = 15,
  //  but on AOSP older sys version, kCollectorTypeHprof maybe 14 / 13 /...
  kCollectorTypeHprof = 15,
};

} // namespace leak_monitor
} // namespace kwai

#endif //KOOM_DEFINES_H_
