package com.kwai.koom.demo.javaleak

import android.app.Application
import com.kwai.koom.base.MonitorLog

import com.kwai.koom.base.MonitorManager
import com.kwai.koom.demo.common.InitTask
import com.kwai.koom.javaoom.monitor.OOMHprofUploader
import com.kwai.koom.javaoom.monitor.OOMMonitor
import com.kwai.koom.javaoom.monitor.OOMMonitorConfig
import com.kwai.koom.javaoom.monitor.OOMReportUploader
import java.io.File

object OOMMonitorInitTask : InitTask {

  override fun init(application: Application) {
    val config = OOMMonitorConfig.Builder()
        .setThreadThreshold(50) //50 only for test! Please use default value!
        .setFdThreshold(300) // 300 only for test! Please use default value!
        .setHeapThreshold(0.9f) // 0.9f for test! Please use default value!
        .setVssSizeThreshold(1_000_000) // 1_000_000 for test! Please use default value!
        .setMaxOverThresholdCount(1) // 1 for test! Please use default value!
        .setAnalysisMaxTimesPerVersion(3) // Consider use default value！
        .setAnalysisPeriodPerVersion(15 * 24 * 60 * 60 * 1000) // Consider use default value！
        .setLoopInterval(5_000) // 5_000 for test! Please use default value!
        .setEnableHprofDumpAnalysis(true)
        .setHprofUploader(object: OOMHprofUploader {
          override fun upload(file: File, type: OOMHprofUploader.HprofType) {
            MonitorLog.e("OOMMonitor", "todo, upload hprof ${file.name} if necessary")
          }
        })
        .setReportUploader(object: OOMReportUploader {
          override fun upload(file: File, content: String) {
            MonitorLog.i("OOMMonitor", content)
            MonitorLog.e("OOMMonitor", "todo, upload report ${file.name} if necessary")
          }
        })
        .build()

    MonitorManager.addMonitorConfig(config)
  }
}