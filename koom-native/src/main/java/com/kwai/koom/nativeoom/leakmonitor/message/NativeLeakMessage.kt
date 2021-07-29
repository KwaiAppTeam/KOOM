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

package com.kwai.koom.nativeoom.leakmonitor.message

import androidx.annotation.Keep
import androidx.annotation.StringDef
import com.google.gson.annotations.SerializedName

@Keep
class NativeLeakMessage {
  @SerializedName("mLeakItems") var leakItems = mutableListOf<NativeLeakItem>()

  @SerializedName("mLogUUID") var logUUID: String = ""

  @SerializedName("mErrorMessage") var errorMessage = "" // 记录过程中碰到的error

  @Retention(AnnotationRetention.SOURCE)
  @StringDef(
      LeakType.TYPE_LEAK_ALLOC,
      LeakType.TYPE_CHECKED_ALLOC
  )
  annotation class LeakType {
    companion object {
      /**
       * Leaked native allocation
       */
      const val TYPE_LEAK_ALLOC = "AndroidLeakAlloc"

      /**
       * Checked native allocation
       */
      const val TYPE_CHECKED_ALLOC = "AndroidCheckedAlloc"
    }
  }

  @Keep
  class NativeLeakItem {
    /**
     * Native allocation exception type[LeakType]
     */
    @LeakType @SerializedName("mType") var type: String? = null

    /**
     * Content of native memory
     */
    @SerializedName("mContent") var content: String? = null

    /**
     * Activity when alloc memory
     */
    @SerializedName("mActivity") var activity: String? = null

    /**
     * Thread of native allocation
     */
    @SerializedName("mThreadName") var threadName: String? = null

    /**
     * Size of native leak
     */
    @SerializedName("mLeakSize") var leakSize: String? = null

    /**
     * Backtrace of native allocation
     */
    @SerializedName("mBacktrace") var backtraceLines = mutableListOf<BacktraceLine>()

    override fun toString() = StringBuilder().apply {
      append("Activity: $activity\n")
      append("LeakSize: $leakSize Byte\n")
      append("LeakType: $type\n")
      append("LeakThread: $threadName\n")
      append("Backtrace:\n")

      for ((index, line) in backtraceLines.withIndex()) {
        append("#$index pc $line\n")
      }
    }.toString()
  }

  @Keep
  class BacktraceLine {
    /**
     * Symbol offset
     */
    @SerializedName("mOffset") var offset: String? = null

    /**
     * So absolute path
     */
    @SerializedName("mSoName") var soName: String? = null

    override fun toString() = "0x$offset  $soName"
  }

  override fun toString() = StringBuilder().apply {
    append("============== Native Leak ${leakItems.size} items ==============\n")

    for ((index, item) in leakItems.withIndex()) {
      append("---------------- LeakItem $index ----------------\n")
      append(item.toString())
      append("\n")
    }
  }.toString()
}