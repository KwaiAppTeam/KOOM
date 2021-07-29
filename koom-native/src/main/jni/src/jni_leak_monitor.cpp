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

#include "android/log.h"
#include "leak_monitor.h"
#include "memory_map.h"
#include <jni.h>
#include <utils/log_util.h>
#include <utils/scoped_local_ref.h>
#include <stdlib.h>

namespace kwai {
namespace leak_monitor {
#define FIND_CLASS(var, class_name)                                                                \
  do {                                                                                             \
    var = env->FindClass(class_name);                                                              \
    CHECK(var);                                                                                    \
  } while (0)

#define GET_METHOD_ID(var, clazz, name, descriptor)                                                \
  do {                                                                                             \
    var = env->GetMethodID(clazz, name, descriptor);                                               \
    CHECK(var);                                                                                    \
  } while (0)

#define GET_FIELD_ID(var, clazz, name, descriptor)                                                 \
  do {                                                                                             \
    var = env->GetFieldID(clazz, name, descriptor);                                                \
    CHECK(var);                                                                                    \
  } while (0)

// TODO：ThreadSafe
static struct {
  jclass global_ref;
  jmethodID construct_method;
  jfieldID index;
  jfieldID size;
  jfieldID thread_name;
  jfieldID so_name;
  jfieldID backtrace;
} g_allocation_info_class;

static struct {
  jclass global_ref;
  jmethodID construct_method;
  jfieldID offset;
  jfieldID so_name;
} g_backtrace_line;

static const uint32_t kT32InstrLen = 2;
static const uint32_t kA32InstrLen = 4;
static const uint32_t kA64InstrLen = 4;
static const uint32_t kNumDropFrame = 1;
static MemoryMap g_memory_map;
static inline jlong GetAdjustPC(jlong pc) {
#if defined(__aarch64__) || defined(__arm__)
  if (pc < kA64InstrLen) {
    return 0;
  }

#if defined(__aarch64__)
  if (pc > kA64InstrLen) {
    pc -= kA64InstrLen;
  }
#else
  if (pc & 1) {
    pc -= kT32InstrLen;
  } else {
    pc -= kA32InstrLen;
  }
#endif
#endif
  return pc;
}

static void InstallMonitor(JNIEnv *env, jclass, jobjectArray selected_list,
                                 jobjectArray ignore_list) {
  int selected_size = selected_list != nullptr ? env->GetArrayLength(selected_list) : 0;
  std::vector<std::string> v_selected_list;
  for (jint i = 0; i < selected_size; i++) {
    jstring str_obj = (jstring)env->GetObjectArrayElement(selected_list, i);
    const char *chr = env->GetStringUTFChars(str_obj, NULL);
    v_selected_list.push_back(chr);
    env->ReleaseStringUTFChars(str_obj, chr);
  }
  // Handle Ignore
  int ignore_size = ignore_list != nullptr ? env->GetArrayLength(ignore_list) : 0;
  std::vector<std::string> v_ignore_list;
  for (jint i = 0; i < ignore_size; i++) {
    // string strObj = (jstring)env->CallObjectMethod(ignore_list, mGet, i);
    jstring str_obj = (jstring)env->GetObjectArrayElement(ignore_list, i);
    const char *chr = env->GetStringUTFChars(str_obj, NULL);
    v_ignore_list.push_back(chr);
    env->ReleaseStringUTFChars(str_obj, chr);
  }
  LeakMonitor::GetInstance().InstallMonitor(&v_selected_list, &v_ignore_list);
}

static void UninstallMonitor(JNIEnv *, jclass) {
  LeakMonitor::GetInstance().UninstallMonitor();
  g_memory_map.~MemoryMap();
}

static void SyncRefreshMonitor(JNIEnv *, jclass) { LeakMonitor::GetInstance().SyncRefresh(); }

static void AsyncRefreshMonitor(JNIEnv *, jclass) { LeakMonitor::GetInstance().AsyncRefresh(); }

static void SetAllocThreshold(JNIEnv *, jclass, jint size) {
  if (size < 0) {
    size = 0;
  }
  LeakMonitor::GetInstance().SetAllocThreshold(size);
}

static jlong GetAllocIndex(JNIEnv *, jclass) {
  return LeakMonitor::GetInstance().CurrentBucketIndex();
}

static void GetLeakAllocs(JNIEnv *env, jclass, jobject allocation_record_map) {
  ScopedLocalRef<jclass> map_class(env, env->GetObjectClass(allocation_record_map));
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
    ScopedLocalRef<jobject> alloc_info_obj(
        env, env->NewObject(g_allocation_info_class.global_ref,
                            g_allocation_info_class.construct_method));
    env->SetIntField(alloc_info_obj.get(), g_allocation_info_class.size,
                     static_cast<jint>(leak_alloc->size));
    env->SetObjectField(alloc_info_obj.get(), g_allocation_info_class.thread_name,
                        ScopedLocalRef<jstring>(env, env->NewStringUTF(leak_alloc->thread_name)).get());
    env->SetLongField(alloc_info_obj.get(), g_allocation_info_class.index,
                      static_cast<jlong>(leak_alloc->index));
    jlong fill[leak_alloc->num_backtraces];
    for (int i = 0; i < leak_alloc->num_backtraces; i++) {
      uintptr_t offset;
      auto *map_entry = g_memory_map.CalculateRelPc(leak_alloc->backtrace[i + kNumDropFrame], &offset);
      DLOGI("map_entry %s", map_entry->name.c_str());
      if (map_entry->NeedIgnore()) {
        leak_alloc->num_backtraces = i;
        break;
      }
      fill[i] = GetAdjustPC(static_cast<jlong>(offset));
      DLOGI("index %d pc %p offset %p", i, leak_alloc->backtrace[i + kNumDropFrame], fill[i]);
    }

    if (!leak_alloc->num_backtraces) {
      continue;
    }

    ScopedLocalRef<jlongArray> pointer_array(env, env->NewLongArray(leak_alloc->num_backtraces));
    DLOGI("num_backtraces %d", leak_alloc->num_backtraces);
    env->SetLongArrayRegion(pointer_array.get(), 0, leak_alloc->num_backtraces, fill);
    env->SetObjectField(alloc_info_obj.get(), g_allocation_info_class.backtrace,
                        pointer_array.get());
    char address[sizeof(uintptr_t) * 2 + 1];
    snprintf(address, sizeof(uintptr_t) * 2 + 1, "%lx", CONFUSE(leak_alloc->address));
    ScopedLocalRef<jstring> memory_address(env, env->NewStringUTF(address));
    ScopedLocalRef<jobject> no_use(env, env->CallObjectMethod(allocation_record_map, put_method, memory_address.get(),
                                                              alloc_info_obj.get()));
  }
}

static const JNINativeMethod kLeakMonitorMethods[] = {
    {"nativeInstallMonitor", "([Ljava/lang/String;[Ljava/lang/String;)V",
     reinterpret_cast<void *>(InstallMonitor)},
    {"nativeUninstallMonitor", "()V", reinterpret_cast<void *>(UninstallMonitor)},
    {"nativeSyncRefreshMonitor", "()V", reinterpret_cast<void *>(SyncRefreshMonitor)},
    {"nativeAsyncRefreshMonitor", "()V", reinterpret_cast<void *>(AsyncRefreshMonitor)},
    {"nativeSetAllocThreshold", "(I)V", reinterpret_cast<void *>(SetAllocThreshold)},
    {"nativeGetAllocIndex", "()J", reinterpret_cast<void *>(GetAllocIndex)},
    {"nativeGetLeakAllocs", "(Ljava/util/Map;)V", reinterpret_cast<void *>(GetLeakAllocs)}};

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env;

  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK) {
    RLOGE("GetEnv Fail!");
    return JNI_ERR;
  }

  jclass leak_monitor;
  FIND_CLASS(leak_monitor, "com/kwai/koom/nativeoom/leakmonitor/LeakMonitor");
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
  if (env->RegisterNatives(leak_monitor, kLeakMonitorMethods, NELEM(kLeakMonitorMethods)) !=
      JNI_OK) {
    RLOGE("RegisterNatives Fail!");
    return JNI_ERR;
  }

  // AllocInfo
  jclass allocation_info;
  FIND_CLASS(allocation_info, "com/kwai/koom/nativeoom/leakmonitor/AllocationInfo");
  g_allocation_info_class.global_ref = reinterpret_cast<jclass>(env->NewGlobalRef(allocation_info));
  GET_METHOD_ID(g_allocation_info_class.construct_method, allocation_info, "<init>", "()V");
  GET_FIELD_ID(g_allocation_info_class.index, allocation_info, "index", "J");
  GET_FIELD_ID(g_allocation_info_class.size, allocation_info, "size", "I");
  GET_FIELD_ID(g_allocation_info_class.thread_name, allocation_info, "threadName",
               "Ljava/lang/String;");
  GET_FIELD_ID(g_allocation_info_class.so_name, allocation_info, "soName",
               "Ljava/lang/String;");
  GET_FIELD_ID(g_allocation_info_class.backtrace, allocation_info, "backtrace", "[J");
  return JNI_VERSION_1_4;
}
} // namespace leak_monitor
} // namespace kwai
