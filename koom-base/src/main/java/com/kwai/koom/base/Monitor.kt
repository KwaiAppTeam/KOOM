package com.kwai.koom.base

abstract class Monitor<C> {
  private var _commonConfig: CommonConfig? = null
  protected val commonConfig: CommonConfig
    get() = _commonConfig!!

  private var _monitorConfig: C? = null
  protected val monitorConfig: C
    get() = _monitorConfig!!

  open var isInitialized = false

  protected inline fun throwIfNotInitialized(
      onDebug: () -> Unit = {
        throw RuntimeException("Monitor is not initialized")
      },
      onRelease: () -> Unit
  ) {
    if (isInitialized) {
      return
    }

    if (MonitorBuildConfig.DEBUG) {
      onDebug()
    } else {
      onRelease()
    }
  }

  protected fun Boolean.syncToInitialized() = apply {
    isInitialized = this && isInitialized
  }

  open fun init(commonConfig: CommonConfig, monitorConfig: C) {
    _commonConfig = commonConfig
    _monitorConfig = monitorConfig

    isInitialized = true
  }

  open fun onApplicationPreAttachContext() = Unit

  open fun onApplicationPostAttachContext() = Unit

  open fun onApplicationPreCreate() = Unit

  open fun onApplicationPostCreate() = Unit

  open fun getLogParams(): Map<String, Any> {
    return mapOf("${javaClass.simpleName.decapitalize()}ingEnabled" to isInitialized)
  }
}