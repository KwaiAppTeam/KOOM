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

  fun addExceptionEventOnSafeMode(message: String, @ExceptionType crashType: Int) = Unit

  @IntDef(
      ExceptionType.UNKNOWN_TYPE,
      ExceptionType.CRASH,
      ExceptionType.EXCEPTION,
      ExceptionType.ANR,
      ExceptionType.NATIVE_CRASH,
      ExceptionType.OOM,
      ExceptionType.FLUTTER_EXCEPTION,
      ExceptionType.OOM_STACKS,
      ExceptionType.ABNORMAL_EXIT,
      ExceptionType.NATIVE_LEAK,
      ExceptionType.MEMORY_MONITOR,
      ExceptionType.FD_STACKS,
      ExceptionType.THREAD_STACKS,
      ExceptionType.LONG_BLOCK,
      ExceptionType.DEAD_LOOP,
      ExceptionType.SYSTEM_EXIT_STAT,
      ExceptionType.METRICS_DIAGNOSTIC_PAYLOAD
  )
  @Retention(AnnotationRetention.SOURCE)
  annotation class ExceptionType {
    companion object {
      const val UNKNOWN_TYPE = 0
      const val CRASH = 1
      const val EXCEPTION = 2
      const val ANR = 3
      const val NATIVE_CRASH = 4
      const val OOM = 5
      const val FLUTTER_EXCEPTION = 6
      const val OOM_STACKS = 7
      const val ABNORMAL_EXIT = 8
      const val NATIVE_LEAK = 9
      const val MEMORY_MONITOR = 10
      const val FD_STACKS = 11
      const val THREAD_STACKS = 12
      const val LONG_BLOCK = 13
      const val DEAD_LOOP = 14
      const val SYSTEM_EXIT_STAT = 15
      const val METRICS_DIAGNOSTIC_PAYLOAD = 16
    }
  }
}