package com.kwai.koom.base

object MonitorLog {
  @JvmStatic
  fun v(tag: String, msg: String): Int {
    return MonitorManager.commonConfig.log.v(tag, msg)
  }

  @JvmStatic
  fun i(tag: String, msg: String): Int {
    return MonitorManager.commonConfig.log.i(tag, msg)
  }

  @JvmStatic
  fun d(tag: String, msg: String): Int {
    return MonitorManager.commonConfig.log.d(tag, msg)
  }

  @JvmStatic
  fun w(tag: String, msg: String): Int {
    return MonitorManager.commonConfig.log.w(tag, msg)
  }

  @JvmStatic
  fun e(tag: String, msg: String): Int {
    return MonitorManager.commonConfig.log.e(tag, msg)
  }

  @JvmStatic
  fun v(tag: String, msg: String, syncToLogger: Boolean): Int {
    if (syncToLogger) MonitorLogger.addCustomStatEvent(tag, msg)

    return this.v(tag, msg)
  }

  @JvmStatic
  fun i(tag: String, msg: String, syncToLogger: Boolean): Int {
    if (syncToLogger) MonitorLogger.addCustomStatEvent(tag, msg)

    return this.i(tag, msg)
  }

  @JvmStatic
  fun d(tag: String, msg: String, syncToLogger: Boolean): Int {
    if (syncToLogger) MonitorLogger.addCustomStatEvent(tag, msg)

    return this.d(tag, msg)
  }

  @JvmStatic
  fun w(tag: String, msg: String, syncToLogger: Boolean): Int {
    if (syncToLogger) MonitorLogger.addCustomStatEvent(tag, msg)

    return this.w(tag, msg)
  }

  @JvmStatic
  fun e(tag: String, msg: String, syncToLogger: Boolean): Int {
    if (syncToLogger) MonitorLogger.addCustomStatEvent(tag, msg)

    return this.e(tag, msg)
  }
}

interface Log {
  fun v(tag: String, msg: String) = runIfDebug { android.util.Log.v(tag, msg) }

  fun i(tag: String, msg: String) = runIfDebug { android.util.Log.i(tag, msg) }

  fun d(tag: String, msg: String) = runIfDebug { android.util.Log.d(tag, msg) }

  fun w(tag: String, msg: String) = runIfDebug { android.util.Log.w(tag, msg) }

  fun e(tag: String, msg: String) = runIfDebug { android.util.Log.e(tag, msg) }
}

internal inline fun runIfDebug(block: () -> Int): Int {
  if (MonitorBuildConfig.DEBUG) {
    return block()
  }

  return -1
}