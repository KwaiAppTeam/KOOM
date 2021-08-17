package com.kwai.performance.overhead.thread.monitor

import android.os.Build
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.isArm64
import com.kwai.koom.base.loop.LoopMonitor

class ThreadMonitor : LoopMonitor<ThreadMonitorConfig>() {
  companion object {
    private const val TAG = "ThreadMonitor"
  }

  @Volatile
  private var mIsRunning = false
  private var mIsNativeInit = false
  private val mResultListener by lazy { monitorConfig.listener }

  private val mNativeCallback = INativeCallback { type, key, value ->
    when (type) {
      Constant.CALL_BACK_TYPE_REPORT -> mResultListener?.onReport(key, value)
      Constant.CALL_BACK_TYPE_CUSTOM_LOG -> {
        // not support
      }
      else -> {
      }
    }
  }

  override fun init(commonConfig: CommonConfig, monitorConfig: ThreadMonitorConfig) {
    super.init(commonConfig, monitorConfig)
    super.startLoop(clearQueue = true, postAtFront = false, delayMillis = monitorConfig.startDelay)
  }

  override fun stopLoop() {
    mIsRunning = false
    super.stopLoop()
  }

  override fun call(): LoopState {
    // Init
    if (!mIsNativeInit) {
      mIsNativeInit = handleNativeInit()
    }
    mIsRunning = true
    // doCheck
    if (mIsNativeInit) {
      handleThreadLeak()
    }
    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  private fun handleThreadLeak() {
    NativeHandler.getInstance().refresh()
  }

  private fun handleNativeInit(): Boolean {
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || Build.VERSION.SDK_INT > Build
            .VERSION_CODES.R) {
      MonitorLog.e(TAG, "koom-thread track not support P below or R above now!")
      return false
    }
    if (!isArm64()) {
      MonitorLog.e(TAG, "koom-thread support arm64 only!")
      return false
    }
    MonitorLog.i(TAG, "koom-thread init")
    try {
      System.loadLibrary("koom-thread")
    } catch (e: Throwable) {
      e.printStackTrace()
      MonitorLog.e(TAG, "loadLibrary exception:" + e.message)
      return false
    }

    NativeHandler.getInstance().setNativeCallback(mNativeCallback)

    if (!BuildConfig.DEBUG) {
      NativeHandler.getInstance().enableSigSegvProtection()
    }

    if (monitorConfig.disableNativeStack) {
      NativeHandler.getInstance().disableNativeStack()
    }
    if (monitorConfig.disableJavaStack) {
      NativeHandler.getInstance().disableJavaStack()
    }

    NativeHandler.getInstance().setThreadLeakDelay(monitorConfig.threadLeakDelay)
    NativeHandler.getInstance().start()
    monitorConfig.listener?.onNativeInit()
    return true
  }
}