/**
 * Copyright 2020 Kwai, Inc. All rights reserved.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.monitor.tracker

import com.kwai.koom.base.MonitorLog
import com.kwai.koom.javaoom.monitor.tracker.model.SystemInfo
import com.kwai.koom.javaoom.monitor.utils.SizeUnit

class FastHugeMemoryOOMTracker : OOMTracker() {

  companion object {
    private const val TAG = "OOMMonitor_FastHugeMemoryTracker"
    private const val REASON_HIGH_WATERMARK = "high_watermark"
    private const val REASON_HUGE_DELTA = "delta"
  }

  private var mDumpReason = ""

  override fun track(): Boolean {
    val javaHeap = SystemInfo.javaHeap

    // 高危阈值直接触发dump分析
    if (javaHeap.rate > monitorConfig.forceDumpJavaHeapMaxThreshold) {
      mDumpReason = REASON_HIGH_WATERMARK
      MonitorLog.i(TAG, "[meet condition] fast huge memory allocated detected, " +
          "high memory watermark, force dump analysis!")
      return true
    }

    // 高差值直接dump
    val lastJavaHeap = SystemInfo.lastJavaHeap
    if (lastJavaHeap.max != 0L && javaHeap.used - lastJavaHeap.used
        > SizeUnit.KB.toByte(monitorConfig.forceDumpJavaHeapDeltaThreshold)) {
      mDumpReason = REASON_HUGE_DELTA
      MonitorLog.i(TAG, "[meet condition] fast huge memory allocated detected, " +
          "over the delta threshold!")
      return true
    }

    return false
  }

  override fun reset() = Unit

  override fun reason() = "reason_fast_huge_$mDumpReason"
}