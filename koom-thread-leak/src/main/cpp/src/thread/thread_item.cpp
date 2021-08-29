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

#include "thread_item.h"

namespace koom {

ThreadItem::ThreadItem() = default;

ThreadItem::ThreadItem(const ThreadItem &threadItem) {
  this->create_time = threadItem.create_time;
  this->id = threadItem.id;
  this->create_call_stack.assign(threadItem.create_call_stack);
  this->thread_detached = threadItem.thread_detached;
  this->thread_internal_id = threadItem.thread_internal_id;
  this->startTime = threadItem.startTime;
  this->exitTime = threadItem.exitTime;
  this->thread_reported = threadItem.thread_reported;
  this->name.assign(threadItem.name);
  this->collect_mode.assign(threadItem.collect_mode);
}

void ThreadItem::Clear() {
  this->id = 0;
  this->create_time = 0;
  this->create_call_stack.clear();
  this->thread_internal_id = 0;
  this->startTime = 0LL;
  this->thread_detached = false;
  this->exitTime = 0LL;
  this->thread_reported = false;
  this->name.clear();
  this->collect_mode.clear();
}
}  // namespace koom