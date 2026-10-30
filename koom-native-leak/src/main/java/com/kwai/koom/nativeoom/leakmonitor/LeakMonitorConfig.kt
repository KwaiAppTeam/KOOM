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

private fun defaultLeakListener() = object : LeakListener {
  override fun onLeak(leaks: MutableCollection<LeakRecord>) {
    leaks.forEach { MonitorLog.i(LeakMonitor.TAG, "$it") }
  }
}

class LeakMonitorConfig(
    val selectedSoList: Array<String>,
    val ignoredSoList: Array<String>,
    val nativeHeapAllocatedThreshold: Int,
    val monitorThreshold: Int,
    val loopInterval: Long,
    val leakListener: LeakListener
) : MonitorConfig<LeakMonitor>() {

  /** Legacy option retained for existing consumers. Defaults to the internal monitor behavior. */
  var enableLocalSymbolic: Boolean = false
    private set

  /** Retained for compatibility with existing consumers. */
  constructor(
      selectedSoList: Array<String>,
      ignoredSoList: Array<String>,
      nativeHeapAllocatedThreshold: Int,
      monitorThreshold: Int,
      loopInterval: Long,
      enableLocalSymbolic: Boolean,
      leakListener: LeakListener
  ) : this(
      selectedSoList,
      ignoredSoList,
      nativeHeapAllocatedThreshold,
      monitorThreshold,
      loopInterval,
      leakListener
  ) {
    this.enableLocalSymbolic = enableLocalSymbolic
  }

  /** Internal monitor-compatible constructor. */
  constructor(
      selectedSoList: Array<String>,
      ignoredSoList: Array<String>,
      leakItemsThreshold: Int,
      nativeHeapAllocatedThreshold: Long,
      mallocThreshold: Int,
      loopInterval: Long,
      samplingHighWatermark: Int,
      samplingLowWatermark: Int
  ) : this(
      selectedSoList = selectedSoList,
      ignoredSoList = ignoredSoList,
      nativeHeapAllocatedThreshold = nativeHeapAllocatedThreshold
          .coerceIn(Int.MIN_VALUE.toLong(), Int.MAX_VALUE.toLong()).toInt(),
      monitorThreshold = mallocThreshold,
      loopInterval = loopInterval,
      leakListener = defaultLeakListener()
  ) {
    this.nativeHeapAllocatedThresholdBytes = nativeHeapAllocatedThreshold
    this.leakItemsThreshold = leakItemsThreshold
    this.samplingHighWatermark = samplingHighWatermark
    this.samplingLowWatermark = samplingLowWatermark
  }

  // Keep the original Int property and constructor for existing consumers.
  var nativeHeapAllocatedThresholdBytes: Long = nativeHeapAllocatedThreshold.toLong()
    private set

  val mallocThreshold: Int
    get() = monitorThreshold

  var leakItemsThreshold: Int = 200
    private set

  var samplingHighWatermark: Int = 270000
    private set
  var samplingLowWatermark: Int = 250000
    private set

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
    private var mNativeHeapAllocatedThreshold = 0L
    private var mLeakItemsThreshold = 200
    private var mSamplingHighWatermark = 270000
    private var mSamplingLowWatermark = 250000
    private var mEnableLocalSymbolic = false

    /**
     * Default is 300s, memory analysis is time consume, NOT below 300s in Production Environment
     */
    private var mLoopInterval = 300_000L

    /**
     * You can receive leaks with your custom leak listener, it run in work thread.
     */
    private var mLeakListener: LeakListener = defaultLeakListener()

    fun setSelectedSoList(selectedSoList: Array<String>) = apply {
      mSelectedSoList = selectedSoList
    }

    fun setIgnoredSoList(ignoredSoList: Array<String>) = apply {
      mIgnoredSoList = ignoredSoList
    }

    fun setNativeHeapAllocatedThreshold(nativeHeapAllocatedThreshold: Int) = apply {
      mNativeHeapAllocatedThreshold = nativeHeapAllocatedThreshold.toLong()
    }

    fun setNativeHeapAllocatedThreshold(nativeHeapAllocatedThreshold: Long) = apply {
      mNativeHeapAllocatedThreshold = nativeHeapAllocatedThreshold
    }

    /** Maximum records per check; internal result packaging emits one item for non-positive values. */
    fun setLeakItemsThreshold(leakItemsThreshold: Int) = apply {
      mLeakItemsThreshold = leakItemsThreshold
    }

    fun setLeakItemThreshold(leakItemsThreshold: Int) =
        setLeakItemsThreshold(leakItemsThreshold)

    fun setSamplingHighWatermark(highWatermark: Int) = apply {
      mSamplingHighWatermark = highWatermark
    }

    fun setSamplingLowWatermark(lowWatermark: Int) = apply {
      mSamplingLowWatermark = lowWatermark
    }

    fun setMonitorThreshold(mallocThreshold: Int) = apply {
      mMonitorThreshold = mallocThreshold
    }

    fun setMallocThreshold(mallocThreshold: Int) =
        setMonitorThreshold(mallocThreshold)

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
        nativeHeapAllocatedThreshold = mNativeHeapAllocatedThreshold
            .coerceIn(Int.MIN_VALUE.toLong(), Int.MAX_VALUE.toLong()).toInt(),
        monitorThreshold = mMonitorThreshold,
        loopInterval = mLoopInterval,
        enableLocalSymbolic = mEnableLocalSymbolic,
        leakListener = mLeakListener
    ).apply {
      nativeHeapAllocatedThresholdBytes = mNativeHeapAllocatedThreshold
      leakItemsThreshold = mLeakItemsThreshold
      samplingHighWatermark = mSamplingHighWatermark
      samplingLowWatermark = mSamplingLowWatermark
    }
  }
}
