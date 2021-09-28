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
import android.util.Log
import androidx.annotation.Keep
import androidx.annotation.RequiresApi
import com.google.gson.Gson
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.Logger
import com.kwai.koom.base.MonitorBuildConfig
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.MonitorLogger
import com.kwai.koom.base.isArm64
import com.kwai.koom.base.loadSoQuietly
import com.kwai.koom.base.loop.LoopMonitor
import com.kwai.koom.nativeoom.leakmonitor.allocationtag.AllocationTagLifecycleCallbacks
import java.lang.RuntimeException
import java.util.*
import java.util.concurrent.atomic.AtomicInteger

@Keep
object LeakMonitor : LoopMonitor<LeakMonitorConfig>() {
  const val TAG = "NativeLeakMonitor"

  @JvmStatic
  private external fun nativeInstallMonitor(selectedList: Array<String>,
    ignoreList: Array<String>, enableLocalSymbolic: Boolean): Boolean

  @JvmStatic
  private external fun nativeUninstallMonitor()

  @JvmStatic
  private external fun nativeSetMonitorThreshold(size: Int)

  @JvmStatic
  private external fun nativeGetAllocIndex(): Long

  @JvmStatic
  private external fun nativeGetLeakAllocs(leakRecordMap: Map<String, LeakRecord>)

  private val mIndex = AtomicInteger()

  private var mIsStart = false

  override fun init(commonConfig: CommonConfig, monitorConfig: LeakMonitorConfig) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N || !isArm64()) {
      MonitorLog.e(TAG, "Native LeakMonitor NOT running in below Android N or Arm 32 bit app")
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
    if (monitorConfig.nativeHeapAllocatedThreshold > 0
        && Debug.getNativeHeapAllocatedSize() > monitorConfig.nativeHeapAllocatedThreshold) {
      return LoopState.Continue
    }

    mutableMapOf<String, LeakRecord>()
      .apply { nativeGetLeakAllocs(this) }
      .also { AllocationTagLifecycleCallbacks.bindAllocationTag(it) }
      .also { MonitorLog.i(TAG, "LeakRecordMap size: ${it.size}") }
      .also { monitorConfig.leakListener.onLeak(it.values) }
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

  @Deprecated("Unfriendly API use start()", ReplaceWith("start"))
  override fun startLoop(clearQueue: Boolean, postAtFront: Boolean, delayMillis: Long) {
    throwIfNotInitialized { return }
    getLoopHandler().post(Runnable {
      if (mIsStart) {
        MonitorLog.e(TAG, "LeakMonitor already start")
        return@Runnable
      }
      mIsStart = true
      if (!nativeInstallMonitor(monitorConfig.selectedSoList,
          monitorConfig.ignoredSoList, monitorConfig.enableLocalSymbolic)) {
        mIsStart = false
        if (MonitorBuildConfig.DEBUG) {
          throw RuntimeException("LeakMonitor Install Fail")
        } else {
          MonitorLog.e(TAG, "LeakMonitor Install Fail")
          return@Runnable
        }
      }

      nativeSetMonitorThreshold(monitorConfig.monitorThreshold)
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
    throwIfNotInitialized { return }
    getLoopHandler().post(Runnable {
      if (!mIsStart) {
        MonitorLog.e(TAG, "LeakMonitor already stop")
        return@Runnable
      }
      mIsStart = false
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
    if (!isInitialized) return
    getLoopHandler().post(Runnable {
      if (!mIsStart) {
        MonitorLog.e(TAG, "Please first start LeakMonitor")
        return@Runnable
      }
      mutableMapOf<String, LeakRecord>()
        .apply { nativeGetLeakAllocs(this) }
        .also { AllocationTagLifecycleCallbacks.bindAllocationTag(it) }
        .also { MonitorLog.i(TAG, "LeakRecordMap size: ${it.size}") }
        .also { monitorConfig.leakListener.onLeak(it.values) }
    })
  }

  /**
   * Only Leak Monitor intern using
   *
   * @return Unique allocation index
   */
  internal fun getAllocationIndex() = nativeGetAllocIndex()
}