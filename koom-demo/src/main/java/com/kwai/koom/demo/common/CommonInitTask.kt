package com.kwai.koom.demo.common

import android.app.Application
import android.os.Build
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.MonitorManager

object CommonInitTask : InitTask {
  override fun init(application: Application) {
    val config = CommonConfig.Builder()
        .setApplication(application) // Set application
        .setVersionNameInvoker { "1.0.0" } // Set version name, java leak feature use it
        .build()

    MonitorManager.initCommonConfig(config)
      .apply { onApplicationCreate() }
  }
}