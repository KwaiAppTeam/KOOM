package com.kwai.koom.demo.common

import android.app.Application
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.Logger
import com.kwai.koom.base.MonitorManager

object CommonInitTask : InitTask {
  override fun init(application: Application) {
    val config = CommonConfig.Builder()
        .setApplication(application)
        .setVersionNameInvoker { "1.0.0" }
        .setRomInvoker { "xxx" }
        .setLogger(object : Logger {})
        .build()

    MonitorManager.initCommonConfig(config)
      .apply { onApplicationCreate() }
  }
}