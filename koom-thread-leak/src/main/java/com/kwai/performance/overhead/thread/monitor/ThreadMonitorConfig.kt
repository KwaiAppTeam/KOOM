package com.kwai.performance.overhead.thread.monitor

import com.kwai.koom.base.MonitorConfig

class ThreadMonitorConfig(val loopInterval: Long,
    val startDelay: Long,
    val disableNativeStack: Boolean, val disableJavaStack: Boolean,
    val threadLeakDelay: Long,
    val enableNativeLog:Boolean,
    val listener: ThreadMonitorLeakListener?) :
    MonitorConfig<ThreadMonitor>() {

  class Builder : MonitorConfig.Builder<ThreadMonitorConfig> {
    private var mListener: ThreadMonitorLeakListener? = null
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

    fun setListener(listener: ThreadMonitorLeakListener) = apply {
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