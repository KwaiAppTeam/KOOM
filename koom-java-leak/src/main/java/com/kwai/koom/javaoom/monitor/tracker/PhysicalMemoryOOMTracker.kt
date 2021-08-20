/**
 * Copyright 2020 Kwai, Inc. All rights reserved.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.monitor.tracker

import com.kwai.koom.base.MonitorLog
import com.kwai.koom.javaoom.monitor.tracker.model.SystemInfo

class PhysicalMemoryOOMTracker : OOMTracker() {

  companion object {
    private const val TAG: String = "OOMMonitor_PhysicalMemoryTracker"
  }

  override fun track(): Boolean {
    val info = SystemInfo.memInfo
    when {
      info.rate < monitorConfig.deviceMemoryThreshold -> {
        MonitorLog.e(TAG, "oom meminfo.rate < " +
            "${monitorConfig.deviceMemoryThreshold * 100}%")
        //return true //先只是上传，不真实触发dump
      }

      info.rate < 0.10f -> {
        MonitorLog.i(TAG, "oom meminfo.rate < 10.0%")
      }

      info.rate < 0.15f -> {
        MonitorLog.i(TAG, "oom meminfo.rate < 15.0%")
      }

      info.rate < 0.20f -> {
        MonitorLog.i(TAG, "oom meminfo.rate < 20.0%")
      }

      info.rate < 0.30f -> {
        MonitorLog.i(TAG, "oom meminfo.rate < 30.0%")
      }
    }
    return false
  }

  override fun reset() = Unit

  override fun reason() = "reason_lmk_oom"

}