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

import android.os.Build
import android.system.Os
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.javaoom.monitor.OOMFileManager
import java.io.File

class FdOOMTracker : OOMTracker() {
  companion object {
    private const val TAG = "OOMMonitor_FdOOMTracker"

    private const val FD_COUNT_THRESHOLD_GAP = 50 //FD连续值递增浮动范围50
  }

  private var mLastFdCount = 0
  private var mOverThresholdCount = 0

  override fun track(): Boolean {
    val fdCount = getFdCount()
    if (fdCount > monitorConfig.fdThreshold && fdCount >= mLastFdCount - FD_COUNT_THRESHOLD_GAP) {
      mOverThresholdCount++

      MonitorLog.i(TAG,
          "[meet condition] "
              + "overThresholdCount: $mOverThresholdCount"
              + ", fdCount: $fdCount")

      dumpFdIfNeed()
    } else {
      reset()
    }

    mLastFdCount = fdCount

    return mOverThresholdCount >= monitorConfig.maxOverThresholdCount
  }

  override fun reset() {
    mLastFdCount = 0
    mOverThresholdCount = 0
  }

  override fun reason() = "reason_fd_oom"

  private fun getFdCount(): Int {
    return File("/proc/self/fd").listFiles()?.size ?: 0
  }

  private fun dumpFdIfNeed() {
    MonitorLog.i(TAG, "over threshold dumpFdIfNeed")

    if (mOverThresholdCount > monitorConfig.maxOverThresholdCount) return

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return

    val fdNames = runCatching { File("/proc/self/fd").listFiles() }
        .getOrElse {
          MonitorLog.i(TAG, "/proc/self/fd child files is empty")

          return@getOrElse emptyArray()
        }
        ?.map { file ->
          runCatching { Os.readlink(file.path) }.getOrElse { "failed to read link ${file.path}" }
        }
        ?: emptyList()

    OOMFileManager.createDumpFile(OOMFileManager.fdDumpDir)
        .run {
          runCatching { writeText(fdNames.sorted().joinToString(separator = ",")) }
        }
  }
}