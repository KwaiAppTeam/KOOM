package com.kwai.koom.demo.nativeleak.init

import android.app.Application
import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.MonitorManager

object CommonInitTask : InitTask {
  override fun init(application: Application) {
    val config = CommonConfig.Builder()
        .setApplication(application)
        .setProductNameInvoker { "kpn" }
        .setVersionNameInvoker { "1.0.0" }
        .setServiceIdInvoker { "UNKNOWN" }
        .setChannelInvoker { "UNKNOWN" }
        .setDeviceIdInvoker { "UNKNOWN" }
        .setRomInvoker { "xxx" }
        .build()

    MonitorManager.initCommonConfig(config)
  }
}