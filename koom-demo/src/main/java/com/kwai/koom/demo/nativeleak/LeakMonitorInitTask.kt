package com.kwai.koom.demo.nativeleak

import android.app.Application
import com.kwai.koom.base.MonitorManager
import com.kwai.koom.demo.common.InitTask
import com.kwai.koom.nativeoom.leakmonitor.LeakMonitorConfig

object LeakMonitorInitTask : InitTask {
  override fun init(application: Application) {

    val config = LeakMonitorConfig.Builder()
        .setLoopInterval(50000) // 设置轮训的间隔
        .setLeakItemThreshold(200) // 收集泄漏的native对象的上限
        .setMonitorThreshold(16) // 设置监听的最小内存值
        .setNativeHeapAllocatedThreshold(0) // 设置native heap分配的内存达到多少阈值开始监控
        .setSelectedSoList(emptyArray()) // 不设置是监控所有， 设置是监听特定的so,  比如监控libcore.so 填写 libcore 不带.so
        .setIgnoredSoList(emptyArray()) // 设置需要忽略监控的so
        .build()

    MonitorManager.addMonitorConfig(config)
  }
}