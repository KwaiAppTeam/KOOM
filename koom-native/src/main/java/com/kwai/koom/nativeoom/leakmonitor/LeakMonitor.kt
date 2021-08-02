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
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.MonitorLogger
import com.kwai.koom.base.loadSoQuietly
import com.kwai.koom.base.loop.LoopMonitor
import com.kwai.koom.nativeoom.leakmonitor.allocationtag.AllocationTagLifecycleCallbacks
import com.kwai.koom.nativeoom.leakmonitor.message.NativeLeakMessage
import java.util.*
import java.util.concurrent.atomic.AtomicInteger

@Keep
object LeakMonitor : LoopMonitor<LeakMonitorConfig>() {
  private const val TAG = "NativeLeakMonitor"
  private const val ERROR_EVENT_KEY = "NativeLeakMonitor_Error"
  private const val NATIVE_LEAK_HAPPENED_BEGIN = "------  Native Leak Found ------\n"
  private const val STOP_LOOP_THRESHOLD = 80000

  private val UUID_PREFIX = UUID.randomUUID().toString()

  @JvmStatic
  private external fun nativeInstallMonitor(selectedList: Array<String>, ignoreList: Array<String>)

  @JvmStatic
  private external fun nativeUninstallMonitor()

  @JvmStatic
  private external fun nativeSyncRefreshMonitor()

  @JvmStatic
  private external fun nativeAsyncRefreshMonitor()

  @JvmStatic
  private external fun nativeSetAllocThreshold(size: Int)

  @JvmStatic
  private external fun nativeGetAllocIndex(): Long

  @JvmStatic
  private external fun nativeGetLeakAllocs(memoryAllocationInfoMap: Map<String, AllocationRecord>)

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

    val allocationInfoMap = mutableMapOf<String, AllocationRecord>()
        .apply { nativeGetLeakAllocs(this) }
        .also { AllocationTagLifecycleCallbacks.bindAllocationTag(it) }
        .also { MonitorLog.i(TAG, "memoryAllocationInfoMap ${it.size}") }

    if (allocationInfoMap.isEmpty()) {
      return LoopState.Continue
    }

    packageLeakMessage(allocationInfoMap).also { uploadLeakMessage(it) }

    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  /**
   * 耗时操作， 可异步
   */
  override fun startLoop(clearQueue: Boolean, postAtFront: Boolean, delayMillis: Long) {
    throwIfNotInitialized { return }

    AllocationTagLifecycleCallbacks.register()

    nativeInstallMonitor(monitorConfig.selectedSoList, monitorConfig.ignoredSoList)
    nativeSetAllocThreshold(monitorConfig.mallocThreshold)

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
   * Sync Refresh Monitor, will hook dynamic loaded libraries.
   */
  fun syncRefresh() {
    if (!isInitialized) return

    nativeSyncRefreshMonitor()
  }

  /**
   * AsyncRefresh Monitor, hook the loaded libraries, but may miss some
   * malloc info at beginning
   */
  fun asyncRefresh() {
    if (!isInitialized) return

    nativeAsyncRefreshMonitor()
  }

  /**
   * Only Leak Monitor intern using
   *
   * @return Unique allocation index
   */
  internal fun getAllocationIndex() = nativeGetAllocIndex()

  @RequiresApi(Build.VERSION_CODES.N)
  private fun packageLeakMessage(
      allocationInfoMap: MutableMap<String, AllocationRecord>
  ): NativeLeakMessage {
    val nativeLeakMessage = NativeLeakMessage().apply {
      logUUID = "${UUID_PREFIX}-${mIndex.getAndIncrement()}"
    }

    if (allocationInfoMap.size > STOP_LOOP_THRESHOLD) {
      stopLoop()
      MonitorLogger.addCustomStatEvent(ERROR_EVENT_KEY,
          "allocationInfoMap size: ${allocationInfoMap.size}")
      return nativeLeakMessage
    }

    runCatching {
      val leakAllocations = LinkedList<Long>()

      loop@ for (allocationInfoEntry in allocationInfoMap) {
          leakAllocations.add(allocationInfoEntry.key.toLong(16))

          val nativeLeakItem = NativeLeakMessage.NativeLeakItem().apply {
            type = NativeLeakMessage.LeakType.TYPE_LEAK_ALLOC
            leakSize = allocationInfoEntry.value.size.toString()
            threadName = allocationInfoEntry.value.threadName
            activity = allocationInfoEntry.value.tag
          }

          for (address in allocationInfoEntry.value.backtrace ?: LongArray(0)) {
            val backtraceLine = NativeLeakMessage.BacktraceLine().apply {
              offset = address.toString(16)
              soName = allocationInfoEntry.value.soName
            }

            nativeLeakItem.backtraceLines.add(backtraceLine)
          }

          if (nativeLeakItem.backtraceLines.size == 0) {
            continue
          }
          nativeLeakMessage.leakItems.add(nativeLeakItem)
          if (nativeLeakMessage.leakItems.size >= monitorConfig.leakItemsThreshold) {
            break@loop
          }
      }
    }.onFailure {
      it.printStackTrace()

      nativeLeakMessage.errorMessage += it

      MonitorLogger.addCustomStatEvent(ERROR_EVENT_KEY, Log.getStackTraceString(it))
    }

    MonitorLog.i(TAG, nativeLeakMessage.toString())
    return nativeLeakMessage
  }

  private fun uploadLeakMessage(leakMessage: NativeLeakMessage) {
    if (leakMessage.leakItems.isEmpty()) {
      return
    }
    runCatching {
      Gson().toJson(leakMessage)
          .also { MonitorLogger.addExceptionEvent(it, Logger.ExceptionType.NATIVE_LEAK) }
          .also { MonitorLog.i(TAG, "$NATIVE_LEAK_HAPPENED_BEGIN$it") }
    }.onFailure {
      it.printStackTrace()

      MonitorLogger.addCustomStatEvent(ERROR_EVENT_KEY, Log.getStackTraceString(it))
    }
  }
}