package com.kwai.koom.demo.threadleak

import android.app.Application
import android.util.Log
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.MonitorManager
import com.kwai.koom.demo.common.InitTask
import com.kwai.performance.overhead.thread.monitor.ThreadLeakListener
import com.kwai.performance.overhead.thread.monitor.ThreadLeakRecord
import com.kwai.performance.overhead.thread.monitor.ThreadMonitor
import com.kwai.performance.overhead.thread.monitor.ThreadMonitorConfig

object ThreadMonitorInitTask : InitTask {
  override fun init(application: Application) {
    val config = ThreadMonitorConfig.Builder()
        .enableThreadLeakCheck(2 * 1000L, 10 * 1000L)
        .setListener(object : ThreadLeakListener {
          override fun onReport(leaks: MutableList<ThreadLeakRecord>) {
            leaks.forEach {
              MonitorLog.i("ThreadMonitor", it.toString())
            }
          }

          override fun onError(msg: String) {
            MonitorLog.e("ThreadMonitor", msg)
          }
        })
        .build()
    MonitorManager.addMonitorConfig(config)
    ThreadMonitor.startTrackAsync()
  }
}