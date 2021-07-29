package com.kwai.koom.base

import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.ProcessLifecycleOwner
import org.json.JSONObject
import java.lang.reflect.ParameterizedType
import java.lang.reflect.Type
import java.util.concurrent.ConcurrentHashMap

object MonitorManager {
  internal val MONITOR_MAP = ConcurrentHashMap<Class<*>, Monitor<*>>()

  internal lateinit var commonConfig: CommonConfig

  @JvmStatic
  fun initCommonConfig(commonConfig: CommonConfig) = apply {
    this.commonConfig = commonConfig
  }

  @JvmStatic
  fun <M : MonitorConfig<*>> addMonitorConfig(config: M) = apply {
    var supperType: Type? = config.javaClass.genericSuperclass
    while (supperType is Class<*>) {
      supperType = supperType.genericSuperclass
    }

    if (supperType !is ParameterizedType) {
      throw java.lang.RuntimeException("config must be parameterized")
    }

    val monitorType = supperType.actualTypeArguments[0] as Class<Monitor<M>>

    if (MONITOR_MAP.containsKey(monitorType)) {
      return@apply
    }

    val monitor = try {
      monitorType.getDeclaredField("INSTANCE").get(null) as Monitor<M>
    } catch (e: Throwable) {
      monitorType.newInstance() as Monitor<M>
    }

    MONITOR_MAP[monitorType] = monitor

    monitor.init(commonConfig, config)

    monitor.logMonitorEvent()
  }

  @JvmStatic
  fun getApplication() = commonConfig.application

  @Deprecated("Use Monitor Directly")
  @JvmStatic
  fun <M : Monitor<*>> getMonitor(clazz: Class<M>): M {
    return MONITOR_MAP[clazz] as M
  }

  @Deprecated("Use Monitor#isInitialized Directly")
  @JvmStatic
  fun <M : Monitor<*>> isInitialized(clazz: Class<M>): Boolean {
    return MONITOR_MAP[clazz] != null
  }

  @JvmStatic
  fun onApplicationPreAttachContext() {
    for (entry in MONITOR_MAP) {
      entry.value.onApplicationPreAttachContext()
    }
  }

  @JvmStatic
  fun onApplicationPostAttachContext() {
    for (entry in MONITOR_MAP) {
      entry.value.onApplicationPostAttachContext()
    }
  }

  @JvmStatic
  fun onApplicationPreCreate() {
    for (entry in MONITOR_MAP) {
      entry.value.onApplicationPreCreate()
    }

    registerApplicationExtension()

    registerMonitorEventObserver()
  }

  @JvmStatic
  fun onApplicationPostCreate() {
    for (entry in MONITOR_MAP) {
      entry.value.onApplicationPostCreate()
    }
  }

  private fun registerMonitorEventObserver() {
    ProcessLifecycleOwner.get().lifecycle.addObserver(object : LifecycleEventObserver {
      private var mHasLogMonitorEvent = false

      override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
        if (event == Lifecycle.Event.ON_START) {
          logAllMonitorEvent()
        }
      }

      private fun logAllMonitorEvent() {
        if (mHasLogMonitorEvent) return
        mHasLogMonitorEvent = true

        mutableMapOf<Any?, Any?>()
            .apply { MONITOR_MAP.forEach { putAll(it.value.getLogParams()) } }
            .also {
              MonitorLogger.addCustomStatEvent("switch-stat", JSONObject(it).toString())
            }
      }
    })
  }

  private fun <C> Monitor<C>.logMonitorEvent() {
    if (!getApplication().isForeground) return

    mutableMapOf<Any?, Any?>()
        .apply { putAll(this@logMonitorEvent.getLogParams()) }
        .also {
          MonitorLogger.addCustomStatEvent("switch-stat", JSONObject(it).toString())
        }
  }
}