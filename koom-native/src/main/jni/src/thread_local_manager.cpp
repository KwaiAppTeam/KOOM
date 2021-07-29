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

#include <sys/types.h>
#include <pthread.h>
#include <sys/mman.h>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <cstring>
#include <cassert>
#include "thread_local_manager.h"

namespace kwai {
namespace leak_monitor {
pthread_key_t ThreadLocalManager::alloc_info_key_ = 0;
pthread_once_t ThreadLocalManager::alloc_info_once_ = PTHREAD_ONCE_INIT;

 int ThreadLocalManager::InitOnce() {
  return pthread_once(&alloc_info_once_, []() -> void {
    pthread_key_create(&alloc_info_key_, [](void *ptr) -> void {
      if (ptr) {
        delete reinterpret_cast<AllocThreadInfo *>(ptr);
      }
    });
  });
}

AllocThreadInfo* ThreadLocalManager::GetAllocThreadInfo() {
  auto ptr = reinterpret_cast<AllocThreadInfo *>(pthread_getspecific(alloc_info_key_));
  if (ptr) {
    return ptr;
  }

  ptr = new AllocThreadInfo;
  assert(ptr != nullptr);
  ptr->cursor = 0;
  if (prctl(PR_GET_NAME, ptr->thread_name)) {
    memcpy(ptr->thread_name, "noname", kMaxThreadNameLen);
  }
  pthread_setspecific(alloc_info_key_, ptr);

  return ptr;
}
} // namespace leak_monitor
} // namespace kwai