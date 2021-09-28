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

import com.kwai.koom.base.MonitorConfig
import com.kwai.koom.base.MonitorLog

class LeakMonitorConfig(
    val selectedSoList: Array<String>,
    val ignoredSoList: Array<String>,
    val nativeHeapAllocatedThreshold: Int,
    val monitorThreshold: Int,
    val loopInterval: Long,
    val enableLocalSymbolic: Boolean,
    val leakListener: LeakListener
) : MonitorConfig<LeakMonitor>() {

  class Builder : MonitorConfig.Builder<LeakMonitorConfig> {
    /**
     * List of so to be monitored
     */
    private var mSelectedSoList = emptyArray<String>()

    /**
     * List of so to be NOT monitored
     */
    private var mIgnoredSoList = emptyArray<String>()

    /**
     * Exceed malloc threshold memory allocation will be monitored
     */
    private var mMonitorThreshold = 16

    /**
     * If Native Heap exceed NativeHeapAllocatedThreshold will trigger leak analysis
     */
    private var mNativeHeapAllocatedThreshold = 0

    /**
     * Default is 300s, memory analysis is time consume, NOT below 300s in Production Environment
     */
    private var mLoopInterval = 300_000L

    /**
     * If enable local symbolic, leak backtrace will contain symbol info, or you only get rel_pc.
     * Then you can use 'address2line' tool analysis rel_pc
     */
    private var mEnableLocalSymbolic = false

    /**
     * You can receive leaks with your custom leak listener, it run in work thread.
     */
    private var mLeakListener: LeakListener = object : LeakListener {
      override fun onLeak(leaks: MutableCollection<LeakRecord>) {
        leaks.forEach { MonitorLog.i(LeakMonitor.TAG, "$it") }
      }
    }

    fun setSelectedSoList(selectedSoList: Array<String>) = apply {
      mSelectedSoList = selectedSoList
    }

    fun setIgnoredSoList(ignoredSoList: Array<String>) = apply {
      mIgnoredSoList = ignoredSoList
    }

    fun setNativeHeapAllocatedThreshold(nativeHeapAllocatedThreshold: Int) = apply {
      mNativeHeapAllocatedThreshold = nativeHeapAllocatedThreshold
    }

    fun setMonitorThreshold(mallocThreshold: Int) = apply {
      mMonitorThreshold = mallocThreshold
    }

    fun setLoopInterval(loopInterval: Long) = apply {
      mLoopInterval = loopInterval
    }

    fun setLeakListener(leakListener: LeakListener) = apply {
      mLeakListener = leakListener
    }

    fun setEnableLocalSymbolic(enableLocalSymbolic: Boolean) = apply {
      mEnableLocalSymbolic = enableLocalSymbolic
    }

    override fun build() = LeakMonitorConfig(
        selectedSoList = mSelectedSoList,
        ignoredSoList = mIgnoredSoList,
        nativeHeapAllocatedThreshold = mNativeHeapAllocatedThreshold,
        monitorThreshold = mMonitorThreshold,
        loopInterval = mLoopInterval,
        enableLocalSymbolic = mEnableLocalSymbolic,
        leakListener = mLeakListener
    )
  }
}