/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <pthread.h>

#include "platform/bionic/macros.h"

class ScopedPthreadMutexLocker {
public:
  explicit ScopedPthreadMutexLocker(pthread_mutex_t *mu) : mu_(mu) { pthread_mutex_lock(mu_); }

  ~ScopedPthreadMutexLocker() { pthread_mutex_unlock(mu_); }

private:
  pthread_mutex_t *mu_;

  BIONIC_DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedPthreadMutexLocker);
};