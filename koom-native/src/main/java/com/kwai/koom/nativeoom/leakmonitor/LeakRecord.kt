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

package com.kwai.koom.nativeoom.leakmonitor

import androidx.annotation.Keep

@Keep
data class LeakRecord(var index: Long,
  var size: Int,
  var threadName: String,
  var frames: Array<FrameInfo>) {
  @JvmField
  var tag: String? = null

  override fun equals(other: Any?): Boolean {
    if (this === other) return true
    if (javaClass != other?.javaClass) return false

    other as LeakRecord

    if (index != other.index) return false
    if (size != other.size) return false
    if (threadName != other.threadName) return false
    if (!frames.contentEquals(other.frames)) return false
    if (tag != other.tag) return false

    return true
  }

  override fun hashCode(): Int {
    var result = index.hashCode()
    result = 31 * result + size
    result = 31 * result + threadName.hashCode()
    result = 31 * result + frames.contentHashCode()
    result = 31 * result + (tag?.hashCode() ?: 0)
    return result
  }

  override fun toString(): String = StringBuilder().apply {
    append("Activity: $tag\n")
    append("LeakSize: $size Byte\n")
    append("LeakThread: $threadName\n")
    append("Backtrace:\n")

    for ((index, line) in frames.withIndex()) {
      append("#$index pc $line\n")
    }
  }.toString()
}

@Keep
data class FrameInfo(var relPc: Long, var soName: String) {
  override fun toString(): String = "0x${relPc.toString(16)}  $soName"
}