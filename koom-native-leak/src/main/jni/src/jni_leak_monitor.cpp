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

#define LOG_TAG "jni_leak_monitor"
#include <jni.h>
#include <jni_util/scoped_local_ref.h>
#include <libgen.h>
#include <log/kcheck.h>
#include <log/log.h>

#include <cstdlib>
#include <vector>

#include "android/log.h"
#include "leak_monitor.h"
#include "memory_map.h"

namespace kwai {
namespace leak_monitor {
#define FIND_CLASS(var, class_name)                      \
  do {                                                   \
    var = env->FindClass(class_name);                    \
    LOG_FATAL_IF(!var, "FindClass %s fail", class_name); \
  } while (0)

#define GET_METHOD_ID(var, clazz, name, descriptor)  \
  do {                                               \
    var = env->GetMethodID(clazz, name, descriptor); \
    LOG_FATAL_IF(!var, "GetMethodID %s fail", name); \
  } while (0)

struct ClassInfo {
  jclass global_ref;
  jmethodID construct_method;
  ClassInfo() : global_ref(nullptr), construct_method(nullptr) {}
};

static ClassInfo g_leak_record;
static ClassInfo g_frame_info;

static const char *kLeakMonitorFullyName =
    "com/kwai/koom/nativeoom/leakmonitor/LeakMonitor";
static const char *kLeakRecordFullyName =
    "com/kwai/koom/nativeoom/leakmonitor/LeakRecord";
static const char *kFrameInfoFullyName =
    "com/kwai/koom/nativeoom/leakmonitor/FrameInfo";
static const uint32_t kNumDropFrame = 2;
static MemoryMap g_memory_map;
static bool g_enable_local_symbolic = false;

static void Clean(JNIEnv *env) {
  if (g_leak_record.global_ref) {
    env->DeleteGlobalRef(g_leak_record.global_ref);
    memset(&g_leak_record, 0, sizeof(g_leak_record));
  }
  if (g_frame_info.global_ref) {
    env->DeleteGlobalRef(g_frame_info.global_ref);
    memset(&g_frame_info, 0, sizeof(g_frame_info));
  }
}

template <typename T>
static inline bool CheckedClean(JNIEnv *env, T value) {
  if (value) {
    return true;
  }
  Clean(env);
  return false;
}

static void UninstallMonitor(JNIEnv *env, jclass) {
  LeakMonitor::GetInstance().Uninstall();
  g_memory_map.~MemoryMap();
  Clean(env);
}

static bool InstallMonitor(JNIEnv *env, jclass clz, jobjectArray selected_array,
                           jobjectArray ignore_array,
                           jboolean enable_local_symbolic) {
  jclass leak_record;
  FIND_CLASS(leak_record, kLeakRecordFullyName);
  g_leak_record.global_ref =
      reinterpret_cast<jclass>(env->NewGlobalRef(leak_record));
  if (!CheckedClean(env, g_leak_record.global_ref)) {
    return false;
  }
  GET_METHOD_ID(g_leak_record.construct_method, leak_record, "<init>",
                "(JILjava/lang/String;[Lcom/kwai/koom/nativeoom/leakmonitor/"
                "FrameInfo;)V");

  jclass frame_info;
  FIND_CLASS(frame_info, kFrameInfoFullyName);
  g_frame_info.global_ref =
      reinterpret_cast<jclass>(env->NewGlobalRef(frame_info));
  if (!CheckedClean(env, g_frame_info.global_ref)) {
    return false;
  }
  GET_METHOD_ID(g_frame_info.construct_method, frame_info, "<init>",
                "(JLjava/lang/String;)V");

  g_enable_local_symbolic = enable_local_symbolic;

  auto array_to_vector =
      [](JNIEnv *env, jobjectArray jobject_array) -> std::vector<std::string> {
    std::vector<std::string> ret;
    int length = env->GetArrayLength(jobject_array);

    if (length <= 0) {
      return ret;
    }

    for (jsize i = 0; i < length; i++) {
      auto str = reinterpret_cast<jstring>(
          env->GetObjectArrayElement(jobject_array, i));
      const char *data = env->GetStringUTFChars(str, nullptr);
      ret.emplace_back(data);
      env->ReleaseStringUTFChars(str, data);
    }

    return std::move(ret);
  };

  std::vector<std::string> selected_so = array_to_vector(env, selected_array);
  std::vector<std::string> ignore_so = array_to_vector(env, ignore_array);
  return CheckedClean(
      env, LeakMonitor::GetInstance().Install(&selected_so, &ignore_so));
}

static void SetMonitorThreshold(JNIEnv *, jclass, jint size) {
  if (size < kDefaultAllocThreshold) {
    size = kDefaultAllocThreshold;
  }
  LeakMonitor::GetInstance().SetMonitorThreshold(size);
}

static jlong GetAllocIndex(JNIEnv *, jclass) {
  return LeakMonitor::GetInstance().CurrentAllocIndex();
}

static jobjectArray BuildFrames(
    JNIEnv *env, std::vector<std::pair<jlong, std::string>> &frames) {
  jsize index = 0;
  jobjectArray frame_array =
      env->NewObjectArray(frames.size(), g_frame_info.global_ref, nullptr);
  for (auto &frame : frames) {
    ScopedLocalRef<jstring> so_name(env,
                                    env->NewStringUTF(frame.second.c_str()));
    ScopedLocalRef<jobject> frame_info(
        env,
        env->NewObject(g_frame_info.global_ref, g_frame_info.construct_method,
                       frame.first, so_name.get()));
    env->SetObjectArrayElement(frame_array, index++, frame_info.get());
  }
  return frame_array;
}

static jobject BuildLeakRecord(JNIEnv *env, uint64_t index, uint32_t size,
                               char *thread_name, jobjectArray frames) {
  ScopedLocalRef<jstring> name(env, env->NewStringUTF(thread_name));
  return env->NewObject(g_leak_record.global_ref,
                        g_leak_record.construct_method, index, size, name.get(),
                        frames);
}

static void GetLeakAllocs(JNIEnv *env, jclass, jobject leak_record_map) {
  ScopedLocalRef<jclass> map_class(env, env->GetObjectClass(leak_record_map));
  jmethodID put_method;
  GET_METHOD_ID(put_method, map_class.get(), "put",
                "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  std::vector<std::shared_ptr<AllocRecord>> leak_allocs =
      LeakMonitor::GetInstance().GetLeakAllocs();

  for (auto &leak_alloc : leak_allocs) {
    if (leak_alloc->num_backtraces <= kNumDropFrame) {
      continue;
    }

    leak_alloc->num_backtraces -= kNumDropFrame;
    std::vector<std::pair<jlong, std::string>> frames;
    for (int i = 0; i < leak_alloc->num_backtraces; i++) {
      uintptr_t offset;
      auto *map_entry = g_memory_map.CalculateRelPc(
          leak_alloc->backtrace[i + kNumDropFrame], &offset);

      if (!map_entry) {
        continue;
      }

      if (map_entry->NeedIgnore()) {
        leak_alloc->num_backtraces = i;
        break;
      }

      std::string symbol_info =
          g_enable_local_symbolic
              ? g_memory_map.FormatSymbol(
                    map_entry, leak_alloc->backtrace[i + kNumDropFrame])
              : basename(map_entry->name.c_str());
      frames.emplace_back(static_cast<jlong>(offset), symbol_info);
    }

    if (!leak_alloc->num_backtraces || frames.empty()) {
      continue;
    }

    char address[sizeof(uintptr_t) * 2 + 1];
    snprintf(address, sizeof(uintptr_t) * 2 + 1, "%lx",
             CONFUSE(leak_alloc->address));
    ScopedLocalRef<jstring> memory_address(env, env->NewStringUTF(address));
    ScopedLocalRef<jobjectArray> frames_ref(env, BuildFrames(env, frames));
    ScopedLocalRef<jobject> leak_record_ref(
        env, BuildLeakRecord(env, leak_alloc->index, leak_alloc->size,
                             leak_alloc->thread_name, frames_ref.get()));
    ScopedLocalRef<jobject> no_use(
        env,
        env->CallObjectMethod(leak_record_map, put_method, memory_address.get(),
                              leak_record_ref.get()));
  }
}

static const JNINativeMethod kLeakMonitorMethods[] = {
    {"nativeInstallMonitor", "([Ljava/lang/String;[Ljava/lang/String;Z)Z",
     reinterpret_cast<void *>(InstallMonitor)},
    {"nativeUninstallMonitor", "()V",
     reinterpret_cast<void *>(UninstallMonitor)},
    {"nativeSetMonitorThreshold", "(I)V",
     reinterpret_cast<void *>(SetMonitorThreshold)},
    {"nativeGetAllocIndex", "()J", reinterpret_cast<void *>(GetAllocIndex)},
    {"nativeGetLeakAllocs", "(Ljava/util/Map;)V",
     reinterpret_cast<void *>(GetLeakAllocs)}};

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env;

  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK) {
    ALOGE("GetEnv Fail!");
    return JNI_ERR;
  }

  jclass leak_monitor;
  FIND_CLASS(leak_monitor, kLeakMonitorFullyName);
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
  if (env->RegisterNatives(leak_monitor, kLeakMonitorMethods,
                           NELEM(kLeakMonitorMethods)) != JNI_OK) {
    ALOGE("RegisterNatives Fail!");
    return JNI_ERR;
  }

  return JNI_VERSION_1_4;
}
}  // namespace leak_monitor
}  // namespace kwai
