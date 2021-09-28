/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author KOOM Team
 *
 */
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
  fun onApplicationCreate() {
    registerApplicationExtension()

    registerMonitorEventObserver()
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