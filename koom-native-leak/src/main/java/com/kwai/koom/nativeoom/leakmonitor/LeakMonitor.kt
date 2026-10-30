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

import android.os.Build
import android.os.Debug
import androidx.annotation.Keep
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.isArm64
import com.kwai.koom.base.loadSoQuietly
import com.kwai.koom.base.loop.LoopMonitor
import com.kwai.koom.nativeoom.leakmonitor.allocationtag.AllocationTagLifecycleCallbacks

@Keep
object LeakMonitor : LoopMonitor<LeakMonitorConfig>() {
  const val TAG = "NativeLeakMonitor"

  @JvmStatic
  private external fun nativeInstallMonitor(selectedList: Array<String>,
    ignoreList: Array<String>, enableLocalSymbolic: Boolean = false): Boolean

  @JvmStatic
  private external fun nativeUninstallMonitor()

  @JvmStatic
  private external fun nativeSetMonitorThreshold(size: Int)

  @JvmStatic
  private external fun nativeSetSamplingWatermark(high: Int, low: Int)

  @JvmStatic
  private external fun nativeGetAllocIndex(): Long

  @JvmStatic
  private external fun nativeGetInterceptMode(): Int

  @JvmStatic
  private external fun nativeGetEverSampled(): Boolean

  @JvmStatic
  private external fun nativeGetLiveAllocCount(): Int

  @JvmStatic
  private external fun nativeGetLeakAllocs(leakRecordMap: Map<String, LeakRecord>, maxLeakCount: Int)

  /** Latest sampling snapshot, refreshed before each LeakListener callback. Null before the first check. */
  @Volatile
  var samplingInfo: LeakSamplingInfo? = null
    private set

  private var mIsStart = false

  override fun init(commonConfig: CommonConfig, monitorConfig: LeakMonitorConfig) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N || !isArm64()) {
      MonitorLog.e(TAG, "Native LeakMonitor requires Android N or later on arm64")
      return
    }
    if (!loadSoQuietly("koom-native")) return

    super.init(commonConfig, monitorConfig)
  }

  /**
   * NOT directly invoke it
   */
  @Deprecated("Unfriendly API use checkLeaks()", ReplaceWith("checkLeaks"))
  override fun call(): LoopState {
    if (monitorConfig.nativeHeapAllocatedThresholdBytes > 0
        && Debug.getNativeHeapAllocatedSize() <= monitorConfig.nativeHeapAllocatedThresholdBytes) {
      return LoopState.Continue
    }

    reportLeaks()
    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  /**
   * Start Leak Monitor, then it will periodically detect leaks
   * Note: time-consuming, usually NOT run in UI thread.
   */
  fun start() {
    startLoop(false, true, monitorConfig.loopInterval)
  }

  /** Legacy no-argument entry point retained for compatibility. */
  @Deprecated("Use start()", ReplaceWith("start()"))
  fun startLoop() = start()

  @Deprecated("Unfriendly API use start()", ReplaceWith("start"))
  override fun startLoop(clearQueue: Boolean, postAtFront: Boolean, delayMillis: Long) {
    if (!isInitialized) {
      MonitorLog.e(TAG, "Please initialize LeakMonitor before start, check init()")
      return
    }
    getLoopHandler().post(Runnable {
      if (mIsStart) {
        MonitorLog.e(TAG, "LeakMonitor already start")
        return@Runnable
      }
      mIsStart = true
      samplingInfo = null
      if (!nativeInstallMonitor(monitorConfig.selectedSoList,
          monitorConfig.ignoredSoList, monitorConfig.enableLocalSymbolic)) {
        mIsStart = false
        MonitorLog.e(TAG, "LeakMonitor Install Fail")
        return@Runnable
      }

      nativeSetMonitorThreshold(monitorConfig.mallocThreshold)
      nativeSetSamplingWatermark(monitorConfig.samplingHighWatermark, monitorConfig.samplingLowWatermark)
      AllocationTagLifecycleCallbacks.register()

      super.startLoop(clearQueue, postAtFront, delayMillis)
    })
  }

  /**
   * Stop Leak Monitor.
   */
  fun stop() {
    stopLoop()
  }

  @Deprecated("Unfriendly API use stop()", ReplaceWith("stop"))
  override fun stopLoop() {
    if (!isInitialized) {
      MonitorLog.e(TAG, "Please initialize LeakMonitor before stop, check init()")
      return
    }
    getLoopHandler().post(Runnable {
      if (!mIsStart) {
        MonitorLog.e(TAG, "LeakMonitor already stop")
        return@Runnable
      }
      mIsStart = false
      samplingInfo = null
      super.stopLoop()
      AllocationTagLifecycleCallbacks.unregister()
      nativeUninstallMonitor()
    })
  }

  /**
   * Directly check leaks, you can receive leak info via LeakListener
   * Note: time-consuming, usually, you don't need directly check leaks
   */
  fun checkLeaks() {
    if (!isInitialized) {
      MonitorLog.e(TAG, "Please initialize LeakMonitor before check, check init()")
      return
    }
    getLoopHandler().post(Runnable {
      if (!mIsStart) {
        MonitorLog.e(TAG, "Please first start LeakMonitor")
        return@Runnable
      }
      reportLeaks()
    })
  }

  private fun reportLeaks() {
    val leaks = mutableMapOf<String, LeakRecord>()
    nativeGetLeakAllocs(leaks, monitorConfig.leakItemsThreshold)
    AllocationTagLifecycleCallbacks.bindAllocationTag(leaks)
    MonitorLog.i(TAG, "leakRecordMap ${leaks.size}")

    samplingInfo = LeakSamplingInfo(
        interceptMode = when (nativeGetInterceptMode()) {
          0 -> LeakInterceptMode.FULL
          1 -> LeakInterceptMode.SAMPLING
          else -> LeakInterceptMode.STOPPED
        },
        everSampled = nativeGetEverSampled(),
        liveAllocCount = nativeGetLiveAllocCount(),
        totalAllocIndex = nativeGetAllocIndex()
    )

    val maxReportedLeakCount = monitorConfig.leakItemsThreshold.takeIf { it > 0 } ?: 1
    val reportedLeaks = leaks.values.take(maxReportedLeakCount).toMutableList()
    runCatching { monitorConfig.leakListener.onLeak(reportedLeaks) }
        .onFailure {
          MonitorLog.e(TAG, "LeakListener.onLeak failed: ${it.message ?: it.javaClass.simpleName}")
        }
  }

  /**
   * Only Leak Monitor intern using
   *
   * @return Unique allocation index
   */
  internal fun getAllocationIndex() = nativeGetAllocIndex()
}

enum class LeakInterceptMode {
  FULL,
  SAMPLING,
  STOPPED
}

data class LeakSamplingInfo(
  val interceptMode: LeakInterceptMode,
  val everSampled: Boolean,
  val liveAllocCount: Int,
  val totalAllocIndex: Long
)
