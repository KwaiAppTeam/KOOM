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
 * Created by shenvsv on 2021.
 *
 */
package com.kwai.performance.overhead.thread.monitor

import com.kwai.koom.base.MonitorConfig

class ThreadMonitorConfig(val loopInterval: Long,
    val startDelay: Long,
    val disableNativeStack: Boolean, val disableJavaStack: Boolean,
    val threadLeakDelay: Long,
    val enableNativeLog:Boolean,
    var listener: ThreadLeakListener?) :
    MonitorConfig<ThreadMonitor>() {

  class Builder : MonitorConfig.Builder<ThreadMonitorConfig> {
    private var mListener: ThreadLeakListener? = null
    private var mLoopInterval = 5 * 1000L
    private var mStartDelay = 0L

    private var disableNativeStack = false
    private var disableJavaStack = false
    private var enableNativeLog = false

    // 线程泄露检测延迟时间
    private var mThreadLeakDelay = 1 * 60 * 1000L //1min

    fun disableNativeStack() = apply {
      disableNativeStack = true
    }

    fun disableJavaStack() = apply {
      disableJavaStack = true
    }

    fun enableNativeLog() = apply {
      enableNativeLog = true
    }

    fun setStartDelay(startDelay: Long) = apply {
      mStartDelay = startDelay
    }

    fun enableThreadLeakCheck(loopInternal: Long, leakDelay: Long) = apply {
      mLoopInterval = loopInternal
      mThreadLeakDelay = leakDelay
    }

    fun setListener(listener: ThreadLeakListener) = apply {
      mListener = listener
    }

    override fun build() = ThreadMonitorConfig(
        loopInterval = mLoopInterval,
        startDelay = mStartDelay,
        disableJavaStack = disableJavaStack,
        disableNativeStack = disableNativeStack,
        threadLeakDelay = mThreadLeakDelay,
        enableNativeLog = enableNativeLog,
        listener = mListener
    )
  }
}