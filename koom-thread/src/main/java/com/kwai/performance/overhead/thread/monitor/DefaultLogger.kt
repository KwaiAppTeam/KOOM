package com.kwai.performance.overhead.thread.monitor

import android.util.Log

internal class DefaultLogger : ThreadMonitorLogger {
  override fun logCustomEvent(key: String, value: String) {
    Log.i(key, value)
  }

  override fun info(key: String, value: String) {
    Log.i(key, value)
  }

  override fun error(key: String, value: String) {
    Log.e(key, value)
  }
}

interface ThreadMonitorLogger {
  fun logCustomEvent(key: String, value: String)
  fun info(key: String, value: String)
  fun error(key: String, value: String)
}