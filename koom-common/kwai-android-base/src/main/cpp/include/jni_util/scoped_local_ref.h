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

#pragma once

#include <jni.h>

template <typename T>
class ScopedLocalRef {
 public:
  ScopedLocalRef(JNIEnv *env, T local_ref) : env_(env), local_ref_(local_ref) {}

  ScopedLocalRef(ScopedLocalRef &&s) : env_(s.env_), local_ref_(s.release()) {}

  ScopedLocalRef &operator=(ScopedLocalRef &&s) noexcept {
    reset(s.release());
    env_ = s.env_;
    return *this;
  }

  explicit ScopedLocalRef(JNIEnv *env) : env_(env), local_ref_(nullptr) {}

  ~ScopedLocalRef() { reset(); }

  void reset(T ptr = nullptr) {
    if (ptr != local_ref_) {
      if (local_ref_) {
        env_->DeleteLocalRef(local_ref_);
      }
      local_ref_ = ptr;
    }
  }

  T release() __attribute__((warn_unused_result)) {
    T local_ref = local_ref_;
    local_ref_ = nullptr;
    return local_ref;
  }

  T get() const { return local_ref_; }

  bool operator==(std::nullptr_t) const { return local_ref_ == nullptr; }

  bool operator!=(std::nullptr_t) const { return local_ref_ != nullptr; }

 private:
  ScopedLocalRef(const ScopedLocalRef &) = delete;
  void operator=(ScopedLocalRef &) = delete;

  JNIEnv *env_;
  T local_ref_;
};