package com.kwai.koom.demo

import android.app.Application
import com.kwai.koom.demo.common.CommonInitTask
import com.kwai.koom.demo.common.InitTask
import com.kwai.koom.demo.javaleak.OOMMonitorInitTask
import com.kwai.koom.demo.threadleak.ThreadMonitorInitTask

object MonitorInitTask : InitTask {
  override fun init(application: Application) {
    CommonInitTask.init(application)
    OOMMonitorInitTask.init(application)
    ThreadMonitorInitTask.init(application)
  }
}