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

class HeapOOMTracker : OOMTracker() {
  companion object {
    private const val TAG = "OOMMonitor_HeapOOMTracker"

    private const val HEAP_RATIO_THRESHOLD_GAP = 0.05f
  }

  private var mLastHeapRatio = 0.0f
  private var mOverThresholdCount = 0

  override fun track(): Boolean {
    val heapRatio = SystemInfo.javaHeap.rate

    if (heapRatio > monitorConfig.heapThreshold
        && heapRatio >= mLastHeapRatio - HEAP_RATIO_THRESHOLD_GAP) {

      mOverThresholdCount++

      MonitorLog.i(TAG,
          "[meet condition] "
              + "overThresholdCount: $mOverThresholdCount"
              + ", heapRatio: $heapRatio"
              + ", usedMem: ${SizeUnit.BYTE.toMB(SystemInfo.javaHeap.used)}mb"
              + ", max: ${SizeUnit.BYTE.toMB(SystemInfo.javaHeap.max)}mb")
    } else {
      reset()
    }

    mLastHeapRatio = heapRatio

    return mOverThresholdCount >= monitorConfig.maxOverThresholdCount
  }

  override fun reset() {
    mLastHeapRatio = 0.0f
    mOverThresholdCount = 0
  }

  override fun reason() = "reason_heap_oom"
}