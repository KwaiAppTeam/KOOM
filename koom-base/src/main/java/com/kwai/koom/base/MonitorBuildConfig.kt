package com.kwai.koom.base

object MonitorBuildConfig {
  @JvmStatic
  val DEBUG by lazy { MonitorManager.commonConfig.debugMode }

  @JvmStatic
  val VERSION_NAME by lazy { MonitorManager.commonConfig.versionNameInvoker() }

  @JvmStatic
  val PRODUCT_NAME by lazy { MonitorManager.commonConfig.productNameInvoker() }

  @JvmStatic
  val SERVICE_ID by lazy { MonitorManager.commonConfig.serviceIdInvoker() }

  @JvmStatic
  val CHANNEL by lazy { MonitorManager.commonConfig.channelInvoker() }

  @JvmStatic
  val DEVICE_ID by lazy { MonitorManager.commonConfig.deviceIdInvoker() }

  @JvmStatic
  val ROM by lazy { MonitorManager.commonConfig.romInvoker() }
}