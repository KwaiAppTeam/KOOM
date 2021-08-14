package com.kwai.performance.overhead.thread.monitor

import com.google.gson.Gson
import com.kwai.koom.base.MonitorConfig

class ThreadMonitorConfig(val loopInterval: Long,
    val versionMaxTriggerTime: Int,
    val startDelay: Long,
    val disableNativeStack: Boolean, val disableJavaStack: Boolean,
    val enableNativeLog: Boolean, val enableThreadAddCustomLog: Boolean,
    val threadLeakInternal: Int, val threadLeakDelay: Long,
    val threadThresholdStart: Int, val threadThresholdStep: Int, val threadThresholdInternal: Int,
    val threadBlockStart: Int, val threadBlockStep: Int, val threadBlockInternal: Int,
    val logger: ThreadMonitorLogger,
    val disableNative:Boolean,
    val gson: Gson,
    val listener: ThreadMonitorResultListener?) :
    MonitorConfig<ThreadMonitor>() {

  class Builder : MonitorConfig.Builder<ThreadMonitorConfig> {
    private var mDisableNative: Boolean = false
    private var mGson: Gson? = null
    private var mListener: ThreadMonitorResultListener? = null
    private var mLogger: ThreadMonitorLogger? = null
    private var mLoopInterval = 5 * 1000L
    private var mVersionMaxTriggerTime = Int.MAX_VALUE
    private var mStartDelay = 0L

    private var disableNativeStack = false
    private var disableJavaStack = false
    private var enableNativeLog = false
    private var enableThreadAddCustomLog = false

    // 线程泄露检测延迟时间
    private var mThreadLeakInternal = 0
    private var mThreadLeakDelay = 1 * 60 * 1000L //1min

    private var mThreadThresholdStart = 0
    private var mThreadThresholdStep = 0
    private var mThreadThresholdInternal = 0

    private var mThreadBlockStart = 0
    private var mThreadBlockStep = 0
    private var mThreadBlockInternal = 0

    fun setLoopInterval(loopInterval: Long) = apply {
      mLoopInterval = loopInterval
    }

    fun disableNativeStack() = apply {
      disableNativeStack = true
    }

    fun disableJavaStack() = apply {
      disableJavaStack = true
    }

    fun enableNativeLog() = apply {
      enableNativeLog = true
    }

    fun enableThreadAddCustomLog() = apply {
      enableThreadAddCustomLog = true
    }

    fun setStartDelay(startDelay: Long) = apply {
      mStartDelay = startDelay
    }

    fun setVersionMaxTriggerTime(time: Int) = apply {
      mVersionMaxTriggerTime = time
    }

    fun enableThreadThresholdCheck(internal: Int, start: Int, step: Int) = apply {
      mThreadThresholdInternal = internal
      mThreadThresholdStart = start
      mThreadThresholdStep = step
    }

    fun enableThreadLeakCheck(internal: Int, leakDelay: Long) = apply {
      mThreadLeakInternal = internal
      mThreadLeakDelay = leakDelay
    }

    // 30s, 4, 4
    fun enableThreadBlockCheck(interval: Int, start: Int, step: Int) = apply {
      mThreadBlockInternal = interval
      mThreadBlockStart = start
      mThreadBlockStep = step
    }

    fun setLogger(logger: ThreadMonitorLogger) = apply {
      mLogger = logger
    }

    fun setListener(listener: ThreadMonitorResultListener) = apply {
      mListener = listener
    }

    fun setGson(gson: Gson) = apply {
      mGson = gson
    }

    fun disableNative() = apply {
      mDisableNative = true
    }

    override fun build() = ThreadMonitorConfig(
        loopInterval = mLoopInterval,
        versionMaxTriggerTime = mVersionMaxTriggerTime,
        startDelay = mStartDelay,
        disableJavaStack = disableJavaStack,
        disableNativeStack = disableNativeStack,
        enableNativeLog = enableNativeLog,
        enableThreadAddCustomLog = enableThreadAddCustomLog,
        threadLeakInternal = mThreadLeakInternal,
        threadLeakDelay = mThreadLeakDelay,
        threadThresholdStart = mThreadThresholdStart,
        threadThresholdStep = mThreadThresholdStep,
        threadThresholdInternal = mThreadThresholdInternal,
        threadBlockStart = mThreadBlockStart,
        threadBlockStep = mThreadBlockStep,
        threadBlockInternal = mThreadBlockInternal,
        logger = mLogger ?: DefaultLogger(),
        gson = mGson?: Gson(),
        disableNative = mDisableNative,
        listener = mListener
    )
  }
}