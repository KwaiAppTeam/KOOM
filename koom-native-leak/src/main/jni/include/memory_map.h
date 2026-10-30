/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_MAP_H_
#define KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_MAP_H_

#include <sys/cdefs.h>

#include "android-base/strings.h"
#include <cstring>
#include <mutex>
#include <set>
#include <string>

// The bundled kwai-android-base shared library does not export EndsWith.
static inline bool MapNameEndsWith(const std::string &name, const char *suffix) {
  const size_t length = std::strlen(suffix);
  return name.size() >= length &&
         name.compare(name.size() - length, length, suffix) == 0;
}

#define LIB_ART "libart.so"
#define OAT_SUFFEX ".oat"
#define ODEX_SUFFEX ".odex"
#define DEX_SUFFEX ".dex"
#define SELF_NAME "libkoom-native.so"
#define HOOK_LIB_NAME "libxhook_lib.so"
#define APP_SO_PREFEX "/data/"

struct MapEntry {
  MapEntry(uintptr_t start, uintptr_t end, uintptr_t offset, const char *name,
           size_t name_len, int flags)
      : start(start),
        end(end),
        offset(offset),
        load_bias(0),
        name(name, name_len),
        flags(flags) {}

  explicit MapEntry(uintptr_t pc) : start(pc), end(pc) {}

  bool NeedSkip() {
    return MapNameEndsWith(name, SELF_NAME) ||
           MapNameEndsWith(name, HOOK_LIB_NAME);
  }

  // FP unwind need terminate when coming to ART frame.
  bool NeedIgnore() {
    return MapNameEndsWith(name, LIB_ART) ||
           MapNameEndsWith(name, OAT_SUFFEX) ||
           MapNameEndsWith(name, ODEX_SUFFEX) ||
           MapNameEndsWith(name, DEX_SUFFEX);
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

struct FrameInfo {
  uint64_t rel_pc;
  uintptr_t load_base;
  std::string name;
  std::string symbol;
  // symbol 对应的起始地址（dladdr 的 dli_saddr），用于格式化 "symbol+offset"；
  // 仅在 FillFrameInfo 成功解析到符号时有效，否则为 0。
  uintptr_t sym_addr = 0;
  std::string build_id;

  FrameInfo() : rel_pc(0), load_base(0) {}
  bool NeedSkip() {
    return MapNameEndsWith(name, SELF_NAME) ||
           MapNameEndsWith(name, HOOK_LIB_NAME);
  }

  bool NeedIgnore() {
    return MapNameEndsWith(name, LIB_ART) ||
           MapNameEndsWith(name, OAT_SUFFEX) ||
           MapNameEndsWith(name, ODEX_SUFFEX) ||
           MapNameEndsWith(name, DEX_SUFFEX);
  }

  bool IsSystemFrame() {
    return !android::base::StartsWith(name, APP_SO_PREFEX);
  }
};

// Ordering comparator that returns equivalence for overlapping entries
struct MapEntryCompare {
  bool operator()(const MapEntry *a, const MapEntry *b) const {
    return a->end <= b->start;
  }
};

class MemoryMap {
 public:
  MemoryMap() = default;
  ~MemoryMap();

  bool FillFrameInfo(uintptr_t pc, FrameInfo &frame_info);
  MapEntry *CalculateRelPc(uintptr_t pc, uintptr_t *rel_pc = nullptr);
  std::string FormatSymbol(MapEntry *entry, uintptr_t pc);
  std::string FormatFrameSymbol(const FrameInfo &frame_info, uintptr_t pc);

 private:
  bool ReadMaps();

  std::set<MapEntry *, MapEntryCompare> entries_;
};

#endif  // KOOM_KOOM_NATIVE_SRC_MAIN_JNI_INCLUDE_MEMORY_MAP_H_
