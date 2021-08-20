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

package com.kwai.koom.javaoom.monitor.tracker.model

import android.os.Build
import android.text.TextUtils
import com.kwai.koom.base.MonitorLog
import java.io.BufferedReader
import java.io.File
import java.io.FileInputStream
import java.io.InputStreamReader
import java.nio.charset.Charset

internal object SystemInfo {
  private const val TAG = "OOMMonitor_SystemInfo"

  private val VSS_REGEX = "VmSize:\\s*(\\d+)\\s*kB".toRegex()
  private val RSS_REGEX = "VmRSS:\\s*(\\d+)\\s*kB".toRegex()
  private val THREADS_REGEX = "Threads:\\s*(\\d+)\\s*".toRegex()
  private val MEM_TOTAL_REGEX = "MemTotal:\\s*(\\d+)\\s*kB".toRegex()
  private val MEM_FREE_REGEX = "MemFree:\\s*(\\d+)\\s*kB".toRegex()
  private val MEM_AVA_REGEX = "MemAvailable:\\s*(\\d+)\\s*kB".toRegex()
  private val MEM_CMA_REGEX = "CmaTotal:\\s*(\\d+)\\s*kB".toRegex()
  private val MEM_ION_REGEX = "ION_heap:\\s*(\\d+)\\s*kB".toRegex()

  var procStatus = ProcStatus()
  var lastProcStatus = ProcStatus()

  var memInfo = MemInfo()
  var lastMemInfo = MemInfo()

  var javaHeap = JavaHeap()
  var lastJavaHeap = JavaHeap()

  //selinux权限问题，先注释掉
  //var dmaZoneInfo: ZoneInfo = ZoneInfo()
  //var normalZoneInfo: ZoneInfo = ZoneInfo()

  fun refresh() {
    lastJavaHeap = javaHeap
    lastMemInfo = memInfo
    lastProcStatus = procStatus

    javaHeap = JavaHeap()
    procStatus = ProcStatus()
    memInfo = MemInfo()

    javaHeap.max = Runtime.getRuntime().maxMemory()
    javaHeap.total = Runtime.getRuntime().totalMemory()
    javaHeap.free = Runtime.getRuntime().freeMemory()
    javaHeap.used = javaHeap.total - javaHeap.free
    javaHeap.rate = 1.0f * javaHeap.used / javaHeap.max

    File("/proc/self/status").forEachLineQuietly { line ->
      if (procStatus.vssInKb != 0 && procStatus.rssInKb != 0
          && procStatus.thread != 0) return@forEachLineQuietly

      when {
        line.startsWith("VmSize") -> {
          procStatus.vssInKb = VSS_REGEX.matchValue(line)
        }

        line.startsWith("VmRSS") -> {
          procStatus.rssInKb = RSS_REGEX.matchValue(line)
        }

        line.startsWith("Threads") -> {
          procStatus.thread = THREADS_REGEX.matchValue(line)
        }
      }
    }

    File("/proc/meminfo").forEachLineQuietly { line ->
      when {
        line.startsWith("MemTotal") -> {
          memInfo.totalInKb = MEM_TOTAL_REGEX.matchValue(line)
        }

        line.startsWith("MemFree") -> {
          memInfo.freeInKb = MEM_FREE_REGEX.matchValue(line)
        }

        line.startsWith("MemAvailable") -> {
          memInfo.availableInKb = MEM_AVA_REGEX.matchValue(line)
        }

        line.startsWith("CmaTotal") -> {
          memInfo.cmaTotal = MEM_CMA_REGEX.matchValue(line)
        }

        line.startsWith("ION_heap") -> {
          memInfo.IONHeap = MEM_ION_REGEX.matchValue(line)
        }
      }
    }

    memInfo.rate = 1.0f * memInfo.availableInKb / memInfo.totalInKb

    MonitorLog.i(TAG, "----OOM Monitor Memory----")
    MonitorLog.i(TAG,"[java] max:${javaHeap.max} used ratio:${(javaHeap.rate * 100).toInt()}%")
    MonitorLog.i(TAG,"[proc] VmSize:${procStatus.vssInKb}kB VmRss:${procStatus.rssInKb}kB " + "Threads:${procStatus.thread}")
    MonitorLog.i(TAG,"[meminfo] MemTotal:${memInfo.totalInKb}kB MemFree:${memInfo.freeInKb}kB " + "MemAvailable:${memInfo.availableInKb}kB")
    MonitorLog.i(TAG,"avaliable ratio:${(memInfo.rate * 100).toInt()}% CmaTotal:${memInfo.cmaTotal}kB ION_heap:${memInfo.IONHeap}kB")
  }

  data class ProcStatus(var thread: Int = 0, var vssInKb: Int = 0, var rssInKb: Int = 0)

  data class MemInfo(var totalInKb: Int = 0, var freeInKb: Int = 0, var availableInKb: Int = 0,
      var IONHeap: Int = 0, var cmaTotal: Int = 0, var rate: Float = 0f)

  data class JavaHeap(var max: Long = 0, var total: Long = 0, var free: Long = 0,
      var used: Long = 0, var rate: Float = 0f)

  private fun Regex.matchValue(s: String) = matchEntire(s.trim())
      ?.groupValues?.getOrNull(1)?.toInt() ?: 0

  private fun File.forEachLineQuietly(charset: Charset = Charsets.UTF_8, action: (line: String) -> Unit) {
    kotlin.runCatching {
      // Note: close is called at forEachLineQuietly
      BufferedReader(InputStreamReader(FileInputStream(this), charset)).forEachLine(action)
    }.onFailure { exception -> exception.printStackTrace() }
  }

  /**
   * 设备是否支持arm64
   */
  fun isSupportArm64(): Boolean {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
      return false
    }

    return supportedAbis().contains("arm64-v8a")
  }

  fun supportedAbis(): Array<String?> {
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
        && Build.SUPPORTED_ABIS.isNotEmpty()) {
      Build.SUPPORTED_ABIS
    } else if (!TextUtils.isEmpty(Build.CPU_ABI2)) {
      arrayOf(Build.CPU_ABI, Build.CPU_ABI2)
    } else {
      arrayOf(Build.CPU_ABI)
    }
  }
}