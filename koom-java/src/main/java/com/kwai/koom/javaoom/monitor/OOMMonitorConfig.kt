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

package com.kwai.koom.javaoom.monitor

import android.os.Build
import com.kwai.koom.base.MonitorBuildConfig
import com.kwai.koom.base.MonitorConfig
import com.kwai.koom.javaoom.monitor.utils.SizeUnit

class OOMMonitorConfig(
    val analysisMaxTimesPerVersion: Int,
    val analysisPeriodPerVersion: Int,

    val heapThreshold: Float,
    val fdThreshold: Int,
    val threadThreshold: Int,
    val deviceMemoryThreshold: Float,
    val maxOverThresholdCount: Int,
    val loopInterval: Long,
    val enableHprofDumpAnalysis: Boolean,
    val forceDumpJavaHeapMaxThreshold: Float,
    val forceDumpJavaHeapDeltaThreshold: Int,
    val hprofUploader: OOMHprofUploader?
) : MonitorConfig<OOMMonitor>() {

  class Builder : MonitorConfig.Builder<OOMMonitorConfig> {
    companion object {
      private val DEFAULT_HEAP_THRESHOLD by lazy {
        // heap阈值
        val maxMem = SizeUnit.BYTE.toMB(Runtime.getRuntime().maxMemory())
        when {
          maxMem >= 512 - 10 /* 误差 */ -> 0.8f
          maxMem >= 256 - 10 /* 误差 */ -> 0.85f
          else -> 0.9f
        }
      }

      private val DEFAULT_THREAD_THRESHOLD by lazy {
        if (MonitorBuildConfig.ROM == "EMUI" && Build.VERSION.SDK_INT <= Build.VERSION_CODES.O) {
          450
        } else {
          750
        }
      }
    }

    private var mAnalysisMaxTimesPerVersion = 5
    private var mAnalysisPeriodPerVersion = 15 * 24 * 60 * 60 * 1000

    private var mHeapThreshold: Float? = null
    private var mVssSizeThreshold = 3_650_000 //单位kb，只适用于32位，64位虚拟机内存无限制
    private var mFdThreshold = 1000
    private var mThreadThreshold: Int? = null
    private var mDeviceMemoryThreshold: Float = 0.05f //整机内存剩余百分比
    private var mForceDumpJavaHeapMaxThreshold = 0.90f //单次轮询，java堆高水位直接dump
    private var mForceDumpJavaHeapDeltaThreshold = 350_000 //单位kb，单次轮询，java heap增加350m时触发dump
    private var mMaxOverThresholdCount = 3
    private var mLoopInterval = 15_000L

    private var mJeChunkHooksVssThreshold = 3_050_000 //单位kb
    private var mJePurgeVssThreshold: Int = 3_250_000 //单位kb
    private var mJePurgeInterval: Int = 12 //例如：一个interval 10s，12个就是120s
    private var mJePurgeMaxTimes: Int = 3
    private var mJeVssRetainedThreshold: Int = 200_000 //单位kb
    private var mJeRssRetainedThreshold: Int = 400_000 //单位kb

    private var mEnableHprofDumpAnalysis = true //内存镜像dump分析
    private var mEnableLowMemoryAutoClean = true //容灾，低内存状态自动清理
    private var mEnableJeMallocHack = false //默认关闭

    private var mHprofUploader: OOMHprofUploader? = null

    fun setAnalysisMaxTimesPerVersion(analysisMaxTimesPerVersion: Int) = apply {
      mAnalysisMaxTimesPerVersion = analysisMaxTimesPerVersion
    }

    fun setAnalysisPeriodPerVersion(analysisPeriodPerVersion: Int) = apply {
      mAnalysisPeriodPerVersion = analysisPeriodPerVersion
    }

    /**
     * @param heapThreshold: 堆内存的使用比例[0.0, 1.0]
     */
    fun setHeapThreshold(heapThreshold: Float) = apply {
      mHeapThreshold = heapThreshold
    }

    /**
     * @param vssSizeThreshold: 单位是kb
     */
    fun setVssSizeThreshold(vssSizeThreshold: Int) = apply {
      mVssSizeThreshold = vssSizeThreshold
    }

    fun setJeChunkHooksVssThreshold(jeChunkHooksVssThreshold: Int) = apply {
      mJeChunkHooksVssThreshold = jeChunkHooksVssThreshold
    }

    fun setJePurgeVssThreshold(jePurgeVssThreshold: Int) = apply {
      mJePurgeVssThreshold = jePurgeVssThreshold
    }

    fun setJePurgeInterval(jePurgeInterval: Int) = apply {
      mJePurgeInterval = jePurgeInterval
    }

    fun setJePurgeMaxTimes(jePurgeMaxTimes: Int) = apply {
      mJePurgeMaxTimes = jePurgeMaxTimes
    }

    fun setJeVssRetainedThreshold(jeVssRetainedThreshold: Int) = apply {
      mJeVssRetainedThreshold = jeVssRetainedThreshold
    }

    fun setJeRssRetainedThreshold(jeRssRetainedThreshold: Int) = apply {
      mJeRssRetainedThreshold = jeRssRetainedThreshold
    }

    fun setFdThreshold(fdThreshold: Int) = apply {
      mFdThreshold = fdThreshold
    }

    fun setThreadThreshold(threadThreshold: Int) = apply {
      mThreadThreshold = threadThreshold
    }

    fun setMaxOverThresholdCount(maxOverThresholdCount: Int) = apply {
      mMaxOverThresholdCount = maxOverThresholdCount
    }

    fun setLoopInterval(loopInterval: Long) = apply {
      mLoopInterval = loopInterval
    }

    fun setEnableHprofDumpAnalysis(enableHprofDumpAnalysis: Boolean) = apply {
      mEnableHprofDumpAnalysis = enableHprofDumpAnalysis
    }

    fun setEnableLowMemoryAutoClean(enableLowMemoryAutoClean: Boolean) = apply {
      mEnableLowMemoryAutoClean = enableLowMemoryAutoClean
    }

    fun setEnableJeMallocHack(enableJeMallocHack: Boolean) = apply {
      mEnableJeMallocHack = enableJeMallocHack;
    }

    fun setDeviceMemoryThreshold(deviceMemoryThreshold: Float) = apply {
      mDeviceMemoryThreshold = deviceMemoryThreshold
    }

    fun setForceDumpJavaHeapDeltaThreshold(forceDumpJavaHeapDeltaThreshold: Int) = apply {
      mForceDumpJavaHeapDeltaThreshold = forceDumpJavaHeapDeltaThreshold
    }

    fun setForceDumpJavaHeapMaxThreshold(forceDumpJavaHeapMaxThreshold: Float) = apply {
      mForceDumpJavaHeapMaxThreshold = forceDumpJavaHeapMaxThreshold
    }

    fun setHprofUploader(hprofUploader: OOMHprofUploader) = apply {
      mHprofUploader = hprofUploader
    }

    override fun build() = OOMMonitorConfig(
        analysisMaxTimesPerVersion = mAnalysisMaxTimesPerVersion,
        analysisPeriodPerVersion = mAnalysisPeriodPerVersion,

        heapThreshold = mHeapThreshold ?: DEFAULT_HEAP_THRESHOLD,
        fdThreshold = mFdThreshold,
        threadThreshold = mThreadThreshold ?: DEFAULT_THREAD_THRESHOLD,
        deviceMemoryThreshold = mDeviceMemoryThreshold,
        maxOverThresholdCount = mMaxOverThresholdCount,
        loopInterval = mLoopInterval,
        enableHprofDumpAnalysis = mEnableHprofDumpAnalysis,

        forceDumpJavaHeapMaxThreshold = mForceDumpJavaHeapMaxThreshold,
        forceDumpJavaHeapDeltaThreshold = mForceDumpJavaHeapDeltaThreshold,

        hprofUploader = mHprofUploader
    )
  }
}