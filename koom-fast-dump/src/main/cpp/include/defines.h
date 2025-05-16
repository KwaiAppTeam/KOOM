//
// Created by zack on 2025/1/26.
//

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
