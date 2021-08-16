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

  override fun init(commonConfig: CommonConfig, monitorConfig: LeakMonitorConfig) {
    if (!loadSoQuietly("native-oom")) return

    super.init(commonConfig, monitorConfig)
  }

  override fun call(): LoopState {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
      return LoopState.Terminate
    }

    if (monitorConfig.nativeHeapAllocatedThreshold > 0
        && Debug.getNativeHeapAllocatedSize() > monitorConfig.nativeHeapAllocatedThreshold) {
      return LoopState.Continue
    }

    val leakRecordMap = mutableMapOf<String, LeakRecord>()
        .apply { nativeGetLeakAllocs(this) }
        .also { AllocationTagLifecycleCallbacks.bindAllocationTag(it) }
        .also { MonitorLog.i(TAG, "LeakRecordMap size: ${it.size}") }

    if (leakRecordMap.isEmpty()) {
      return LoopState.Continue
    }

    monitorConfig.leakListener.onLeak(leakRecordMap.values)
    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  /**
   * 耗时操作， 可异步
   */
  override fun startLoop(clearQueue: Boolean, postAtFront: Boolean, delayMillis: Long) {
    throwIfNotInitialized { return }

    AllocationTagLifecycleCallbacks.register()

    if (!nativeInstallMonitor(monitorConfig.selectedSoList,
        monitorConfig.ignoredSoList, monitorConfig.enableLocalSymbolic)) {
      if (MonitorBuildConfig.DEBUG) {
        throw RuntimeException("LeakMonitor Install Fail")
      } else {
        MonitorLog.e(TAG, "LeakMonitor Install Fail")
        return
      }
    }
    nativeSetMonitorThreshold(monitorConfig.monitorThreshold)

    super.startLoop(clearQueue, postAtFront, delayMillis)
  }

  /**
   * 耗时操作， 可异步
   */
  override fun stopLoop() {
    throwIfNotInitialized { return }

    AllocationTagLifecycleCallbacks.unregister()

    super.stopLoop()
    nativeUninstallMonitor()
  }

  /**
   * Only Leak Monitor intern using
   *
   * @return Unique allocation index
   */
  internal fun getAllocationIndex() = nativeGetAllocIndex()
}