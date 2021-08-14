package com.kwai.performance.overhead.thread.monitor.utils

import android.text.TextUtils
import com.google.gson.annotations.SerializedName
import com.kwai.performance.overhead.thread.monitor.ThreadBlockReport
import java.io.File

/**
 * 获取进程的Vss，Rss等状态
 */
fun getProcessMemoryUsage(): ProcessStatus {
  val usage = ProcessStatus()
  val file = File("/proc/self/status")
  try {
    file.forEachLine { line ->
      if (TextUtils.isEmpty(line)) {
        return@forEachLine
      }
      if (line.startsWith("VmSize") && line.contains("kB")) {
        val strings = line.split("\\s+".toRegex()).toTypedArray()
        if (strings.size > 1) {
          usage.vssKbSize = strings[1].toLong()
        }
      } else if (line.startsWith("VmRSS:") && line.contains("kB")) {
        val strings = line.split("\\s+".toRegex()).toTypedArray()
        if (strings.size > 1) {
          usage.rssKbSize = strings[1].toLong()
        }
      } else if (line.startsWith("Threads:")) {
        val strings = line.split("\\s+".toRegex()).toTypedArray()
        if (strings.size > 1) {
          usage.threadsCount = strings[1].toInt()
        }
      }
    }
  } catch (e: Exception) {
  }
  return usage
}

/**
 * string化一个调用栈
 *
 * @param tabCount StackTraceElement缩进的TAB数
 */
fun getStackTrace(stackTraceElements: Array<StackTraceElement>?, tabCount: Int): String {
  if (stackTraceElements == null) {
    return ""
  }
  val sb = StringBuilder()
  for (traceElement in stackTraceElements) {
    for (j in 0 until tabCount) {
      sb.append("\t")
    }
    sb.append("at ").append(traceElement).append('\n')
  }
  return sb.substring(0)
}

private val THREAD_ROOT_DIR = "/proc/self/task"

data class ProcThreadItem(@SerializedName("tid") val tid: String, @SerializedName("name") val name: String)

fun getProcThreads(): List<ProcThreadItem>? {
  return File(THREAD_ROOT_DIR).listFiles()?.mapNotNull {
    val tid = it.name
    val name: String? = try {
      File(it, "comm").readLines().firstOrNull()
    } catch (e: Exception) {
      null
    }
    if (!name.isNullOrEmpty()) {
      ProcThreadItem(tid, name)
    } else {
      null
    }
  }
}

data class SimpleThreadData(
    @SerializedName("leakType") val leakType: String,
    @SerializedName("procSize") val procSize: Int,
    @SerializedName("procList") val procList: List<ProcThreadItem>,
    @SerializedName("threadSize") val threadSize: Int,
    @SerializedName("threadList") val threadList: List<ThreadBlockReport>)

fun getSimpleThreadData(type: String): SimpleThreadData {
  val procList = getProcThreads() ?: emptyList()
//  val threadList = Thread.getAllStackTraces().map {
//    val thread = it.key
//    ThreadBlockReport(thread.id, thread.state, thread.name, getStackTrace(it.value, 0), 0)
//  }
  val threadList = emptyList<ThreadBlockReport>()
  return SimpleThreadData(type, procList.size, procList, threadList.size, threadList)
}

class ProcessStatus {
  var totalByteSize: Long = 0
  var vssKbSize: Long = 0
  var rssKbSize: Long = 0
  var pssKbSize: Long = 0
  var javaHeapByteSize: Long = 0
  var threadsCount = 0
}