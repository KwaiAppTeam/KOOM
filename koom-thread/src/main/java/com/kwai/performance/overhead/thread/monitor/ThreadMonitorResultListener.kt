package com.kwai.performance.overhead.thread.monitor

interface ThreadMonitorResultListener {
  fun onReport(type: String, msg: String)
  fun onReportSimple(type: String, msg: String)
  fun onError(type: String, error: Throwable?)
  fun onNativeInit()
}