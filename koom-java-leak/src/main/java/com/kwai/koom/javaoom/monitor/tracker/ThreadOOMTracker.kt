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
import com.kwai.koom.base.MonitorManager.getApplication
import com.kwai.koom.base.currentActivity
import com.kwai.koom.javaoom.monitor.OOMFileManager
import com.kwai.koom.javaoom.monitor.tracker.model.SystemInfo
import java.io.File

class ThreadOOMTracker : OOMTracker() {
  companion object {
    private const val TAG = "OOMMonitor_ThreadOOMTracker"

    private const val THREAD_COUNT_THRESHOLD_GAP = 50 //Thread连续值递增浮动范围50
  }

  private var mLastThreadCount = 0
  private var mOverThresholdCount = 0

  override fun track(): Boolean {
    val threadCount = getThreadCount()

    if (threadCount > monitorConfig.threadThreshold
        && threadCount >= mLastThreadCount - THREAD_COUNT_THRESHOLD_GAP) {
      mOverThresholdCount++

      MonitorLog.i(TAG,
          "[meet condition] "
              + "overThresholdCount:$mOverThresholdCount"
              + ", threadCount: $threadCount")

      dumpThreadIfNeed()
    } else {
      reset()
    }

    mLastThreadCount = threadCount

    return mOverThresholdCount >= monitorConfig.maxOverThresholdCount
  }

  override fun reset() {
    mLastThreadCount = 0
    mOverThresholdCount = 0
  }

  override fun reason() = "reason_thread_oom"

  private fun getThreadCount(): Int {
    return SystemInfo.procStatus.thread
  }

  private fun dumpThreadIfNeed() {
    MonitorLog.i(TAG, "over threshold dumpThreadIfNeed")

    if (mOverThresholdCount > monitorConfig.maxOverThresholdCount) return

    val threadNames = runCatching { File("/proc/self/task").listFiles() }
        .getOrElse {
          MonitorLog.i(TAG, "/proc/self/task child files is empty")

          return@getOrElse emptyArray()
        }
        ?.map {
          runCatching { File(it, "comm").readText() }.getOrElse { "failed to read $it/comm" } }
        ?.map {
          if (it.endsWith("\n")) it.substring(0, it.length - 1) else it
        }
        ?: emptyList()

    MonitorLog.i(TAG, "threadNames = $threadNames")

    OOMFileManager.createDumpFile(OOMFileManager.threadDumpDir)
        .run {
          runCatching { writeText(threadNames.joinToString(separator = ",")) }
        }
  }
}