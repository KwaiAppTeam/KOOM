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

import androidx.annotation.IntDef

object MonitorLogger : Logger {
  override fun addCustomStatEvent(key: String, value: String?, realtimeReport: Boolean) {
    MonitorManager.commonConfig.logger.addCustomStatEvent(key, value, realtimeReport)
  }

  override fun addExceptionEvent(message: String, crashType: Int) {
    MonitorManager.commonConfig.logger.addExceptionEvent(message, crashType)
  }
}

interface Logger {
  fun addCustomStatEvent(key: String, value: String?, realtimeReport: Boolean = false) = Unit

  fun addExceptionEvent(message: String, @ExceptionType crashType: Int) = Unit

  @IntDef(
      ExceptionType.UNKNOWN_TYPE,
      ExceptionType.OOM,
      ExceptionType.OOM_STACKS,
      ExceptionType.NATIVE_LEAK,
      ExceptionType.THREAD_STACKS
  )
  @Retention(AnnotationRetention.SOURCE)
  annotation class ExceptionType {
    companion object {
      const val UNKNOWN_TYPE = 0
      const val OOM = 1
      const val OOM_STACKS = 2
      const val NATIVE_LEAK = 3
      const val THREAD_STACKS = 4
    }
  }
}