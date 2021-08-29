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

#include <fast_unwind/fast_unwind.h>
#include <fast_unwind/fast_unwind_test.h>
#include <kwai_util/kwai_macros.h>
#include <kwai_util/ktime.h>
#include <linux/prctl.h>
#include <log/log_main.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <atomic>
#include <cstring>

#define LOG_TAG "unwind"

#define MAX_TASK_NAME_LEN (16)

map_t *create_map(int size) {
  auto *t = (map_t *)malloc(sizeof(map_t));
  t->size = size;
  t->list = (map_node_t **)calloc(sizeof(map_node_t *), size);
  t->list_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * size);
  for (int i = 0; i < size; i++) {
    t->list_mutex[i] = PTHREAD_MUTEX_INITIALIZER;
  }
  return t;
}

void free_map(map_t *t) {
  for (int i = 0; i < t->size; i++) {
    map_node_t *current = t->list[i];
    while (current != nullptr) {
      map_node_t *to_be_free = current;
      current = current->next;
      free(to_be_free->malloc_allocation);
      free(to_be_free);
    }
  }

  free(t->list_mutex);
  free(t->list);
  free(t);
}

int hash_code(map_t *t, long long int key) { return key % t->size; }

void insert_in_map(map_t *t, long long int key, malloc_allocation_t *malloc_allocation) {
  int pos = hash_code(t, key);

  pthread_mutex_lock(&t->list_mutex[pos]);

  map_node_t *list = t->list[pos];
  map_node_t *temp = list;
  while (temp) {
    if (temp->key == key) {
      free(temp->malloc_allocation);
      temp->malloc_allocation = malloc_allocation;
      pthread_mutex_unlock(&t->list_mutex[pos]);
      return;
    }
    temp = temp->next;
  }

  auto *new_node = (map_node_t *)malloc(sizeof(map_node_t));
  new_node->key = key;
  new_node->malloc_allocation = malloc_allocation;
  new_node->next = list;
  t->list[pos] = new_node;
  pthread_mutex_unlock(&t->list_mutex[pos]);
}

malloc_allocation_list_node_t *dump_all_allocation_in_map(map_t *t) {
  malloc_allocation_list_node_t *list_head = nullptr;
  malloc_allocation_list_node_t *list_tail = nullptr;

  for (int i = 0; i < t->size; i++) {
    pthread_mutex_lock(&t->list_mutex[i]);

    map_node_t *current_node = t->list[i];
    while (current_node != nullptr && current_node->malloc_allocation != nullptr &&
           current_node->malloc_allocation->allocation_size > 0) {
      auto *new_node =
          (malloc_allocation_list_node_t *)malloc(sizeof(malloc_allocation_list_node_t));

      if (list_head == nullptr) {
        list_head = new_node;
        list_tail = list_head;
      } else {
        list_tail->next_node = new_node;
        list_tail = new_node;
      }
      list_tail->next_node = nullptr;
      auto *malloc_allocation = (malloc_allocation_t *)malloc(sizeof(malloc_allocation_t));

      malloc_allocation->index = current_node->malloc_allocation->index;
      malloc_allocation->allocation_size = current_node->malloc_allocation->allocation_size;
      malloc_allocation->memory_address = current_node->malloc_allocation->memory_address;
      malloc_allocation->backtrace_frame_count =
          current_node->malloc_allocation->backtrace_frame_count;
      memcpy(malloc_allocation->backtrace, current_node->malloc_allocation->backtrace,
             sizeof(const void *) * MAX_BACKTRACE_FRAMES);

      list_tail->current_allocation = malloc_allocation;
      current_node = current_node->next;
    }

    pthread_mutex_unlock(&t->list_mutex[i]);
  }
  return list_head;
}

malloc_allocation_t *remove_in_map(map_t *t, long long int key) {
  int pos = hash_code(t, key);

  pthread_mutex_lock(&t->list_mutex[pos]);

  map_node_t *list = t->list[pos];
  map_node_t *temp = list;
  map_node_t *pre = nullptr;

  while (temp) {
    if (temp->key == key) {
      if (pre == nullptr) {
        t->list[pos] = temp->next;
      } else {
        pre->next = temp->next;
      }
      malloc_allocation_t *malloc_allocation = temp->malloc_allocation;
      free(temp);
      pthread_mutex_unlock(&t->list_mutex[pos]);
      return malloc_allocation;
    }
    pre = temp;
    temp = temp->next;
  }
  pthread_mutex_unlock(&t->list_mutex[pos]);
  return nullptr;
}

static std::atomic_llong s_all_memory_allocation;
static std::atomic_llong s_live_memory_allocation;
static std::atomic_size_t s_allocation_index = 1;

static pthread_mutex_t s_allocation_index_mutex = PTHREAD_MUTEX_INITIALIZER;

static const int kAllocationMapItemCount = 100000;
static map_t *s_allocation_map = nullptr;

static void fill_backtrace(malloc_allocation_t *allocation_pointer, const uintptr_t *buffer,
                           size_t frames_count) {
  for (int i = 0; i < frames_count; i++) {
    allocation_pointer->backtrace[i] = buffer[i];
  }
}

KWAI_EXPORT void init_memory_allocation() {
  s_allocation_map = create_map(kAllocationMapItemCount);

  s_all_memory_allocation = 0;
  s_live_memory_allocation = 0;
  s_allocation_index = 1;
}

KWAI_EXPORT void add_memory_allocation(void *memory_address, size_t size) {
  uintptr_t buffer[MAX_BACKTRACE_FRAMES]{};
  char thread_name[MAX_TASK_NAME_LEN + 1]; // one more for termination
  if (prctl(PR_GET_NAME, reinterpret_cast<unsigned long>(thread_name), 0, 0, 0) != 0) {
    strcpy(thread_name, "<name unknown>");
  }
  int64_t start = nanotime();
  size_t frames = fast_unwind(buffer, MAX_BACKTRACE_FRAMES);
  if (frames <= 0) {
    ALOGE("%s malloc %d byte unwind failed！", thread_name, size);
    return;
  }
  int64_t end = nanotime();
  ALOGI("%s malloc %d byte at %p unwind [%d] frames cost %lld ns", thread_name, size,
        memory_address, frames, (end - start));

  auto *malloc_allocation =
      reinterpret_cast<malloc_allocation_t *>(malloc(sizeof(malloc_allocation_t)));

  malloc_allocation->allocation_size = size;
  malloc_allocation->memory_address = memory_address;
  malloc_allocation->backtrace_frame_count = frames;
  fill_backtrace(malloc_allocation, buffer, frames);

  s_all_memory_allocation += malloc_allocation->allocation_size;
  s_live_memory_allocation += malloc_allocation->allocation_size;

  pthread_mutex_lock(&s_allocation_index_mutex);
  malloc_allocation->index = s_allocation_index++;
  pthread_mutex_unlock(&s_allocation_index_mutex);

  insert_in_map(s_allocation_map, (long long int)memory_address, malloc_allocation);
}

KWAI_EXPORT void remove_memory_allocation(void *memory_address) {
  malloc_allocation_t *malloc_allocation =
      remove_in_map(s_allocation_map, (long long int)memory_address);

  if (malloc_allocation != nullptr) {
    s_live_memory_allocation -= malloc_allocation->allocation_size;
    free(malloc_allocation);
  }
}

KWAI_EXPORT malloc_allocation_list_node_t *get_all_malloc_allocations() {
  return dump_all_allocation_in_map(s_allocation_map);
}

KWAI_EXPORT void free_all_malloc_allocation(malloc_allocation_list_node_t *list_header) {
  malloc_allocation_list_node_t *current_node = list_header;
  malloc_allocation_list_node_t *next_node;
  while (current_node != nullptr) {
    next_node = current_node->next_node;

    free(current_node->current_allocation);
    free(current_node);

    current_node = next_node;
  }
}

long long int get_all_allocation_size() { return s_all_memory_allocation; }

long long int get_live_allocation_size() { return s_live_memory_allocation; }

long long int get_allocation_index() { return s_allocation_index; }