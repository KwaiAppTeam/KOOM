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

import android.app.ActivityManager
import android.content.Context
import java.io.File
import java.util.regex.Pattern

private val VSS_REGEX = "VmSize:\\s*(\\d+)\\s*kB".toRegex()
private val RSS_REGEX = "VmRSS:\\s*(\\d+)\\s*kB".toRegex()
private val THREADS_REGEX = "Threads:\\s*(\\d+)\\s*".toRegex()

private val MEM_TOTAL_REGEX = "MemTotal:\\s*(\\d+)\\s*kB".toRegex()
private val MEM_FREE_REGEX = "MemFree:\\s*(\\d+)\\s*kB".toRegex()
private val MEM_AVA_REGEX = "MemAvailable:\\s*(\\d+)\\s*kB".toRegex()
private val MEM_CMA_REGEX = "CmaTotal:\\s*(\\d+)\\s*kB".toRegex()
private val MEM_ION_REGEX = "ION_heap:\\s*(\\d+)\\s*kB".toRegex()

private var mCpuCoreCount: Int? = null
private var mRamTotalSize: Long? = null
private var mCpuMaxFreq: Double? = null

@JvmField
var lastProcessStatus = ProcessStatus()

@JvmField
var lastMemInfo = MemInfo()

@JvmField
var lastJavaHeap = JavaHeap()

fun getRamTotalSize(): Long {
  return mRamTotalSize ?: File("/proc/meminfo").useLines {
    it.forEach { line ->
      if (line.contains("MemTotal")) {
        val array = line.split("\\s+".toRegex()).toTypedArray()
        return@useLines array.getOrElse(1) { "0" }.toLong() shl 10
      }
    }
    return@useLines 0L
  }.also { mRamTotalSize = it }
}

fun getRamAvailableSize(context: Context): Long {
  val memoryInfo = ActivityManager.MemoryInfo()

  (context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager)
      .also { it.getMemoryInfo(memoryInfo) }

  return memoryInfo.availMem
}

fun getCpuCoreCount(): Int {
  return mCpuCoreCount
      ?: runCatching {
        File("/sys/devices/system/cpu/")
            .listFiles { pathname -> Pattern.matches("cpu[0-9]+", pathname.name) }
            ?.size
            ?: 0
      }.getOrDefault(Runtime.getRuntime().availableProcessors()).also { mCpuCoreCount = it }
}

fun getCpuMaxFreq(): Double {
  return mCpuMaxFreq
      ?: runCatching {
        (File("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq")
            .readFirstLine()
            ?.trim()
            ?.toDouble()
            ?: 0.0) / 1000
      }.getOrDefault(0.0).also { mCpuMaxFreq = it }
}

/**
 * Get Pss/Vss/etc.
 */
fun getProcessStatus(): ProcessStatus {
  val processStatus = ProcessStatus()
  runCatching {
    File("/proc/self/status").useLines {
      it.forEach { line ->
        if (processStatus.vssKbSize != 0L && processStatus.rssKbSize != 0L
            && processStatus.threadsCount != 0L) {
          lastProcessStatus = processStatus
          return processStatus
        }
        when {
          line.startsWith("VmSize") -> processStatus.vssKbSize = VSS_REGEX.matchValue(line)
          line.startsWith("VmRSS") -> processStatus.rssKbSize = RSS_REGEX.matchValue(line)
          line.startsWith("Threads") ->
            processStatus.threadsCount = THREADS_REGEX.matchValue(line)
        }
      }
    }
  }
  lastProcessStatus = processStatus
  return processStatus
}

fun getMemoryInfo(): MemInfo {
  val memInfo = MemInfo()
  File("/proc/meminfo").useLines {
    it.forEach { line ->
      when {
        line.startsWith("MemTotal") -> memInfo.totalInKb = MEM_TOTAL_REGEX.matchValue(line)
        line.startsWith("MemFree") -> memInfo.freeInKb = MEM_FREE_REGEX.matchValue(line)
        line.startsWith("MemAvailable") -> memInfo.availableInKb = MEM_AVA_REGEX.matchValue(line)
        line.startsWith("CmaTotal") -> memInfo.cmaTotal = MEM_CMA_REGEX.matchValue(line)
        line.startsWith("ION_heap") -> memInfo.IONHeap = MEM_ION_REGEX.matchValue(line)
      }
    }
  }
  memInfo.rate = 1.0f * memInfo.availableInKb / memInfo.totalInKb
  lastMemInfo = memInfo
  return memInfo
}

fun getJavaHeap(): JavaHeap {
  val javaHeap = JavaHeap()
  javaHeap.max = Runtime.getRuntime().maxMemory()
  javaHeap.total = Runtime.getRuntime().totalMemory()
  javaHeap.free = Runtime.getRuntime().freeMemory()
  javaHeap.used = javaHeap.total - javaHeap.free
  javaHeap.rate = 1.0f * javaHeap.used / javaHeap.max
  lastJavaHeap = javaHeap
  return javaHeap
}

class ProcessStatus {
  @JvmField
  var vssKbSize: Long = 0

  @JvmField
  var rssKbSize: Long = 0

  @JvmField
  var threadsCount: Long = 0
}

class MemInfo(
    @JvmField
    var totalInKb: Long = 0,
    @JvmField
    var freeInKb: Long = 0,
    @JvmField
    var availableInKb: Long = 0,
    @JvmField
    var IONHeap: Long = 0,
    @JvmField
    var cmaTotal: Long = 0,
    @JvmField
    var rate: Float = 0f
)

class JavaHeap(
    @JvmField
    var max: Long = 0,
    @JvmField
    var total: Long = 0,
    @JvmField
    var free: Long = 0,
    @JvmField
    var used: Long = 0,
    @JvmField
    var rate: Float = 0f
)

private fun Regex.matchValue(s: String) = matchEntire(s.trim())
    ?.groupValues?.getOrNull(1)?.toLong() ?: 0L