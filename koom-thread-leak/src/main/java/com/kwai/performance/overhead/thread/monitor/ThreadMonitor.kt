package com.kwai.performance.overhead.thread.monitor

import android.os.Build
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.isArm64
import com.kwai.koom.base.loadSoQuietly
import com.kwai.koom.base.loop.LoopMonitor

object ThreadMonitor : LoopMonitor<ThreadMonitorConfig>() {
  private const val TAG = "koom-thread-monitor"

  @Volatile
  private var mIsRunning = false

  private val mResultListener by lazy { monitorConfig.listener }

  private val mNativeCallback = INativeCallback { type, key, value ->
    when (type) {
      Constant.CALL_BACK_TYPE_REPORT -> mResultListener?.onReport(key, value)
      else -> {}
    }
  }

  fun startTrack() {
    if (handleNativeInit()) {
      mIsRunning = true
      startLoop(clearQueue = true, postAtFront = false, delayMillis = monitorConfig.startDelay)
    }
  }

  fun startTrackAsync() {
    getLoopHandler().postAtFrontOfQueue {
      startTrack()
    }
  }

  fun stop() {
    if (mIsRunning) {
      NativeHandler.getInstance().stop()
    }
    stopLoop()
  }

  override fun call(): LoopState {
    handleThreadLeak()
    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  private fun handleThreadLeak() {
    MonitorLog.i(TAG, "handleThreadLeak")
    NativeHandler.getInstance().refresh()
  }

  private fun handleNativeInit(): Boolean {
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || Build.VERSION.SDK_INT > Build
            .VERSION_CODES.R) {
      MonitorLog.e(TAG, "not support P below or R above now!")
      return false
    }
    if (!isArm64()) {
      MonitorLog.e(TAG, "support arm64 only!")
      return false
    }
    if (loadSoQuietly("koom-thread")) {
      MonitorLog.i(TAG, "loadLibrary success")
    } else {
      MonitorLog.e(TAG, "loadLibrary fail")
      return false
    }
    if (monitorConfig.disableNativeStack) {
      NativeHandler.getInstance().disableNativeStack()
    }
    if (monitorConfig.disableJavaStack) {
      NativeHandler.getInstance().disableJavaStack()
    }
    if (monitorConfig.enableNativeLog) {
      NativeHandler.getInstance().enableNativeLog()
    }
    NativeHandler.getInstance().setNativeCallback(mNativeCallback)
    NativeHandler.getInstance().setThreadLeakDelay(monitorConfig.threadLeakDelay)
    NativeHandler.getInstance().start()
    MonitorLog.i(TAG, "init finish")
    return true
  }
}