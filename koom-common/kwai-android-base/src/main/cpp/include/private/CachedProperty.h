/*
 * Copyright (C) 2017 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <string.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

// Cached system property lookup. For code that needs to read the same property multiple times,
// this class helps optimize those lookups.
class CachedProperty {
public:
  // The lifetime of `property_name` must be greater than that of this CachedProperty.
  explicit CachedProperty(const char *property_name)
      : property_name_(property_name), prop_info_(nullptr), cached_area_serial_(0),
        cached_property_serial_(0), is_read_only_(strncmp(property_name, "ro.", 3) == 0),
        read_only_property_(nullptr) {
    cached_value_[0] = '\0';
  }

  // Returns true if the property has been updated (based on the serial rather than the value)
  // since the last call to Get.
  bool DidChange() {
    uint32_t initial_property_serial_ = cached_property_serial_;
    Get();
    return (cached_property_serial_ != initial_property_serial_);
  }

  // Returns the current value of the underlying system property as cheaply as possible.
  // The returned pointer is valid until the next call to Get. It is the caller's responsibility
  // to provide a lock for thread-safety.
  const char *Get() {
    // Do we have a `struct prop_info` yet?
    if (prop_info_ == nullptr) {
      // `__system_property_find` is expensive, so only retry if a property
      // has been created since last time we checked.
      uint32_t property_area_serial = __system_property_area_serial();
      if (property_area_serial != cached_area_serial_) {
        prop_info_ = __system_property_find(property_name_);
        cached_area_serial_ = property_area_serial;
      }
    }

    if (prop_info_ != nullptr) {
      // Only bother re-reading the property if it's actually changed since last time.
      uint32_t property_serial = __system_property_serial(prop_info_);
      if (property_serial != cached_property_serial_) {
#if __ANDROID_API__ >= 26
        __system_property_read_callback(prop_info_, &CachedProperty::Callback, this);
#endif /* __ANDROID_API__ >= 26 */
      }
    }
    if (is_read_only_ && read_only_property_ != nullptr) {
      return read_only_property_;
    }
    return cached_value_;
  }

private:
  const char *property_name_;
  const prop_info *prop_info_;
  uint32_t cached_area_serial_;
  uint32_t cached_property_serial_;
  char cached_value_[PROP_VALUE_MAX];
  bool is_read_only_;
  const char *read_only_property_;

  static void Callback(void *data, const char *, const char *value, uint32_t serial) {
    CachedProperty *instance = reinterpret_cast<CachedProperty *>(data);
    instance->cached_property_serial_ = serial;
    // Read only properties can be larger than PROP_VALUE_MAX, but also never change value or
    // location, thus we return the pointer from the shared memory directly.
    if (instance->is_read_only_) {
      instance->read_only_property_ = value;
    } else {
      strlcpy(instance->cached_value_, value, PROP_VALUE_MAX);
    }
  }
};