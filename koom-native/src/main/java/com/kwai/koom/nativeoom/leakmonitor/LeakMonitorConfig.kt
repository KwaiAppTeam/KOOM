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

class LeakMonitorConfig(
    val selectedSoList: Array<String>,
    val ignoredSoList: Array<String>,

    val leakItemsThreshold: Int,
    val nativeHeapAllocatedThreshold: Int,
    val mallocThreshold: Int,
    val loopInterval: Long
) : MonitorConfig<LeakMonitor>() {

  class Builder : MonitorConfig.Builder<LeakMonitorConfig> {
    private var mSelectedSoList = emptyArray<String>()
    private var mIgnoredSoList = emptyArray<String>()

    private var mLeakItemsThreshold = 200
    private var mMallocThreshold = 7
    private var mNativeHeapAllocatedThreshold = 0

    /**
     * Default is 300s, memory analysis is time consume, NOT below 300s in Production Environment
     */
    private var mLoopInterval = 300_000L

    fun setSelectedSoList(selectedSoList: Array<String>) = apply {
      mSelectedSoList = selectedSoList
    }

    fun setIgnoredSoList(ignoredSoList: Array<String>) = apply {
      mIgnoredSoList = ignoredSoList
    }

    fun setLeakItemThreshold(leakItemsThreshold: Int) = apply {
      mLeakItemsThreshold = leakItemsThreshold
    }

    fun setNativeHeapAllocatedThreshold(nativeHeapAllocatedThreshold: Int) = apply {
      mNativeHeapAllocatedThreshold = nativeHeapAllocatedThreshold
    }

    fun setMallocThreshold(mallocThreshold: Int) = apply {
      mMallocThreshold = mallocThreshold
    }

    fun setLoopInterval(loopInterval: Long) = apply {
      mLoopInterval = loopInterval
    }

    override fun build() = LeakMonitorConfig(
        selectedSoList = mSelectedSoList,
        ignoredSoList = mIgnoredSoList,

        leakItemsThreshold = mLeakItemsThreshold,
        nativeHeapAllocatedThreshold = mNativeHeapAllocatedThreshold,
        mallocThreshold = mMallocThreshold,
        loopInterval = mLoopInterval
    )
  }
}