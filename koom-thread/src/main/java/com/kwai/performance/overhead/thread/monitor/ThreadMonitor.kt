package com.kwai.performance.overhead.thread.monitor

import android.os.Build
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.loop.LoopMonitor
import com.kwai.performance.overhead.thread.monitor.utils.getProcessMemoryUsage
import com.kwai.performance.overhead.thread.monitor.utils.getSimpleThreadData

class ThreadMonitor : LoopMonitor<ThreadMonitorConfig>() {
  companion object {
    private const val TAG = "ThreadMonitor"

    // TODO 用户监控启动阶段的时候，提前hook
    fun preInit(startCollect: Boolean): Boolean {
      try {
        System.loadLibrary("koom-thread")
        NativeHandler.getInstance().start()
        if (startCollect) {
          NativeHandler.getInstance().startCollect("pre_init")
        }
        return true
      } catch (e: Throwable) {
        e.printStackTrace()
      }
      return false
    }
  }

  @Volatile
  private var mIsRunning = false
  private var mIsNativeInit = false
  private var mLoopTimes = 0L
  private var mThreadThresholdLimit = 0
  private val mResultListener by lazy { monitorConfig.listener }
  private val mThreadBlockChecker by lazy { ThreadBlockChecker(monitorConfig) }

  private val mNativeCallback = INativeCallback { type, key, value ->
    when (type) {
      Constant.CALL_BACK_TYPE_REPORT -> mResultListener?.onReport(key, value)
      Constant.CALL_BACK_TYPE_CUSTOM_LOG -> monitorConfig.logger.logCustomEvent(key, value)
      else -> {
      }
    }
  }

  override fun init(commonConfig: CommonConfig, monitorConfig: ThreadMonitorConfig) {
    super.init(commonConfig, monitorConfig)
    mThreadThresholdLimit = monitorConfig.threadThresholdStart
  }

  override fun onApplicationPostCreate() {
    super.onApplicationPostCreate()
//    startTrack()
  }

  override fun startLoop(clearQueue: Boolean, postAtFront: Boolean, delayMillis: Long) {
    if (BuildConfig.DEBUG) {
      throw Exception("use startTrack not startLoop!")
    }
  }

  override fun stopLoop() {
    mIsRunning = false
    super.stopLoop()
  }

  fun startTrack() {
//    KVData.init(commonConfig.application)
//    val version = KVData.appVersion()
//    val times = KVData.getTriggerTimes(version)
//    if (times > monitorConfig.versionMaxTriggerTime) {
//      monitorConfig.logger.error(TAG, "koom-thread already triggered max times!")
//      return
//    }
    monitorConfig.logger.info(TAG, "koom-thread start with delay ${monitorConfig.startDelay}")
    //load so库，初始化thread hook参数
    super.startLoop(clearQueue = true, postAtFront = false, delayMillis = monitorConfig.startDelay)
  }

  override fun call(): LoopState {
    // Init
    if (!mIsNativeInit) {
      mIsNativeInit = handleNativeInit()
    }
    mIsRunning = true
    // doCheck
    handleThreadThreshold()
    if (mIsNativeInit) {
      handleThreadLeak()
    }
    mThreadBlockChecker.handleBlockCheck(mLoopTimes)
    mLoopTimes++
    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  fun report(type: String, delay: Long) {
    monitorConfig.logger.info(TAG, "report start $delay")
    getLoopHandler().postDelayed({
      if (mIsRunning) {
        doReportThread(type)
      }
    }, delay)
  }

  fun startCollect(mode: String) {
    getLoopHandler().post {
      if (mIsRunning && mIsNativeInit) {
        NativeHandler.getInstance().startCollect(mode)
      }
    }
  }

  fun endCollect() {
    getLoopHandler().post {
      if (mIsRunning && mIsNativeInit) {
        NativeHandler.getInstance().endCollect()
      }
    }
  }

  private fun handleThreadLeak() {
    if (monitorConfig.threadLeakInternal <= 0 || mLoopTimes % monitorConfig.threadLeakInternal
        != 0L) {
      return
    }
    NativeHandler.getInstance().refresh()
  }

  private fun handleNativeInit(): Boolean {
    if (monitorConfig.disableNative) {
      return false
    }
    // 小于 o 或 大于 r 的版本没测试过，忽略
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || Build.VERSION.SDK_INT > Build
            .VERSION_CODES.R) {
      monitorConfig.logger.error(TAG, "koom-thread track not support P below or R above now!")
      return false
    }
    monitorConfig.logger.info(TAG, "koom init")
    try {
      System.loadLibrary("koom-thread")
    } catch (e: Throwable) {
      e.printStackTrace()
      monitorConfig.logger.logCustomEvent(TAG, "loadLibrary exception:" + e.message)
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
    if (monitorConfig.enableNativeLog) {
      NativeHandler.getInstance().enableNativeLog()
    }

    if (monitorConfig.enableThreadAddCustomLog) {
      NativeHandler.getInstance().enableThreadAddCustomLog()
    }

    NativeHandler.getInstance().setThreadLeakDelay(monitorConfig.threadLeakDelay)
    NativeHandler.getInstance().start()
    monitorConfig.listener?.onNativeInit()
    return true
  }

  private fun handleThreadThreshold() {
    monitorConfig.logger.info(TAG, "handleThreadPoll")
    if (mThreadThresholdLimit <= 0) {
      return
    }
    if (monitorConfig.threadThresholdInternal <= 0 || mLoopTimes % monitorConfig
            .threadThresholdInternal != 0L) {
      return
    }
    getProcessMemoryUsage().threadsCount.let { threadCount ->
      monitorConfig.logger.info(TAG,
          "handleThreadPoll Thread size:$threadCount THREAD_OVER_THRESHOLD_COUNT:${mThreadThresholdLimit}")
      if (threadCount > mThreadThresholdLimit) {
        monitorConfig.logger.info(TAG, "reportThreadData")
        doReportThread("over_limit")
        if (monitorConfig.threadThresholdStep > 0) {
          mThreadThresholdLimit += monitorConfig.threadThresholdStep
        } else {
          mThreadThresholdLimit = 0
        }
      }
    }
  }

  private fun doReportThread(type: String) {
    if (mIsNativeInit) {
      NativeHandler.getInstance().logThreadStatus(type)
    } else {
      monitorConfig.listener?.onReportSimple(type, monitorConfig.gson.toJson(getSimpleThreadData
      ("simple_$type")))
    }
  }
}