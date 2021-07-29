package com.kwai.koom.base

import android.app.ActivityManager
import android.content.Context
import android.os.Process
import java.io.File

private var mProcessName: String? = null

fun isMainProcess() = MonitorManager.getApplication().packageName == getProcessName()

fun getProcessName(): String? {
  return mProcessName
      ?: getProcessNameByAms()?.also { mProcessName = it }
      ?: getProcessNameByProc()?.also { mProcessName = it }
}

private fun getProcessNameByProc(): String? {
  return try {
    File("/proc/" + Process.myPid() + "/" + "cmdline").readText().trim(' ', '\u0000')
  } catch (e: Exception) {
    e.printStackTrace()
    null
  }
}

private fun getProcessNameByAms(): String? {
  try {
    val activityManager = MonitorManager.getApplication().getSystemService(Context.ACTIVITY_SERVICE)
        as ActivityManager

    val appProcessList = activityManager.runningAppProcesses
    for (processInfo in appProcessList.orEmpty()) {
      if (processInfo.pid == Process.myPid()) {
        return processInfo.processName
      }
    }
  } catch (e: Exception) {
    e.printStackTrace()
  }
  return null
}