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

  open fun getLogParams(): Map<String, Any> {
    return mapOf("${javaClass.simpleName.decapitalize()}ingEnabled" to isInitialized)
  }
}