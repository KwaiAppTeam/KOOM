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

#ifndef KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_MAP_H_
#define KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_MAP_H_

#include <sys/cdefs.h>

#include <mutex>
#include <set>
#include <string>

const std::string kArtSo = "libart.so";
const std::string kSoSuffex = ".so";

struct MapEntry {
  MapEntry(uintptr_t start,
           uintptr_t end,
           uintptr_t offset,
           const char *name,
           size_t name_len,
           int flags)
      : start(start), end(end), offset(offset), name(name, name_len), flags(flags) {}

  explicit MapEntry(uintptr_t pc) : start(pc), end(pc) {}

  bool NeedIgnore() {
    auto ends_with = [](std::string &target, const std::string &suffix) -> bool {
      return target.size() >= suffix.size() &&
          target.substr(target.size() - suffix.size(), suffix.size()) == suffix;
    };
    return ends_with(name, kArtSo) || !ends_with(name, kSoSuffex);
  }

  uintptr_t start;
  uintptr_t end;
  uintptr_t offset;
  uintptr_t load_bias;
  uintptr_t elf_start_offset = 0;
  std::string name;
  int flags;
  bool init = false;
  bool valid = false;
};

// Ordering comparator that returns equivalence for overlapping entries
struct MapEntryCompare {
  bool operator()(const MapEntry *a, const MapEntry *b) const { return a->end <= b->start; }
};

class MemoryMap {
 public:
  MemoryMap() = default;
  ~MemoryMap();

  MapEntry *CalculateRelPc(uintptr_t pc, uintptr_t *rel_pc = nullptr);
  std::string FormatBacktrace(const uintptr_t* frames, size_t frame_count);
 private:
  bool ReadMaps();

  std::set<MapEntry *, MapEntryCompare> entries_;
};

#endif //KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_MAP_H_
