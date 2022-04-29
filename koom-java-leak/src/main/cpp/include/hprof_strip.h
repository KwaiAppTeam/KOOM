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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2021.
 *
 */

#ifndef KOOM_HPROF_STRIP_H
#define KOOM_HPROF_STRIP_H

#include <android-base/macros.h>

#include <memory>
#include <string>

namespace kwai {
namespace leak_monitor {

class HprofStrip {
 public:
  static HprofStrip &GetInstance();
  static void HookInit();
  int HookOpenInternal(const char *path_name, int flags, ...);
  ssize_t HookWriteInternal(int fd, const void *buf, ssize_t count);
  bool IsHookSuccess() const;
  void SetHprofName(const char *hprof_name);

 private:
  HprofStrip();
  ~HprofStrip() = default;
  DISALLOW_COPY_AND_ASSIGN(HprofStrip);

  static int GetShortFromBytes(const unsigned char *buf, int index);
  static int GetIntFromBytes(const unsigned char *buf, int index);
  static int GetByteSizeFromType(unsigned char basic_type);

  int ProcessHeap(const void *buf, int first_index, int max_len,
                  int heap_serial_no, int array_serial_no);

  static size_t FullyWrite(int fd, const void *buf, ssize_t count);
  void reset();

  int hprof_fd_;
  int strip_bytes_sum_;
  int heap_serial_num_;
  int hook_write_serial_num_;
  int strip_index_;

  bool is_hook_success_;
  bool is_current_system_heap_;

  std::string hprof_name_;

  static constexpr int kStripListLength = 65536 * 2 * 2 + 2;
  int strip_index_list_pair_[kStripListLength];
};

}  // namespace leak_monitor
}  // namespace kwai

#endif  // KOOM_HPROF_STRIP_H