package com.kwai.koom.demo.javaleak

import android.app.Application

import com.kwai.koom.base.MonitorManager
import com.kwai.koom.demo.common.InitTask
import com.kwai.koom.javaoom.monitor.OOMMonitor
import com.kwai.koom.javaoom.monitor.OOMMonitorConfig

object OOMMonitorInitTask : InitTask {

  override fun init(application: Application) {
    val config = OOMMonitorConfig.Builder()
        .setThreadThreshold(50) // 线程数量的阈值
        .setFdThreshold(750) // 文件描述符数量的阈值
        .setHeapThreshold(0.9f) // 堆内存的比例值上限
        .setVssSizeThreshold(1_000_000) // vss的阈值大小，1G for test
        .setMaxOverThresholdCount(1) // 线程、文件描述符、堆内存、vss 连续多次超过此值则会触发dump，
        .setAnalysisMaxTimesPerVersion(5) // 每个版本最多分析的次数
        .setAnalysisPeriodPerVersion(15 * 24 * 60 * 60 * 1000) // 每个版本最长分析多久
        .setLoopInterval(1_000) // 轮询的间隔
        .setEnableHprofDumpAnalysis(true)
        .build()

    MonitorManager.addMonitorConfig(config)
    OOMMonitor.startLoop(delayMillis = 5_000L)
  }
}