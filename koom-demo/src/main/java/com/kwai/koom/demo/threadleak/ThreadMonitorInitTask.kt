package com.kwai.koom.demo.threadleak

import android.app.Application
import android.util.Log
import com.kwai.koom.base.MonitorManager
import com.kwai.koom.demo.common.InitTask
import com.kwai.performance.overhead.thread.monitor.ThreadMonitor
import com.kwai.performance.overhead.thread.monitor.ThreadMonitorConfig
import com.kwai.performance.overhead.thread.monitor.ThreadMonitorResultListener

object ThreadMonitorInitTask : InitTask {
  override fun init(application: Application) {
    val config = ThreadMonitorConfig.Builder()
        .enableNativeLog()
        .setLoopInterval(2 * 1000)
//        .setStartDelay(5 * 1000)
//        .disableNative()
//        .enableThreadAddCustomLog()
        .enableThreadLeakCheck(1, 10 * 1000)
        .setListener(object : ThreadMonitorResultListener {
          override fun onReport(type: String, msg: String) {
            Log.i(type, msg)
          }

          override fun onReportSimple(type: String, msg: String) {
          }

          override fun onError(type: String, error: Throwable?) {
          }

          override fun onNativeInit() {
          }
        })
        .build()

    MonitorManager.addMonitorConfig(config)
    val monitor = MonitorManager.getMonitor(ThreadMonitor::class.java)
    monitor.startTrack()
  }
}