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
import android.os.Build
import android.os.Process
import com.kwai.koom.base.MonitorManager.getApplication
import java.io.File

enum class Abi {
  ARMEABI_V7A,
  ARM64_V8A,
  UNKNOWN
}

@Volatile
private var mCurrentAbi: Abi = Abi.UNKNOWN
private var mProcessName: String? = null

fun isMainProcess() = MonitorManager.getApplication().packageName == getProcessName()

fun getProcessName(): String? {
  return mProcessName
      ?: getProcessNameByAms()?.also { mProcessName = it }
      ?: getProcessNameByProc()?.also { mProcessName = it }
}

fun isArm64(): Boolean {
  return getCurrentAbi() == Abi.ARM64_V8A
}

fun getCurrentAbi(): Abi {
  if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return Abi.ARMEABI_V7A

  if (mCurrentAbi != Abi.UNKNOWN) return mCurrentAbi

  // Check system api
  "dalvik.system.VMRuntime".toClass()
    ?.callStaticMethod<Any>("getRuntime")
    ?.run { callMethod<Boolean>("is64Bit") }
    ?.also {
      mCurrentAbi = if (it) Abi.ARM64_V8A else Abi.ARMEABI_V7A
      return mCurrentAbi
    }

  // Check address size
  "sun.misc.Unsafe".toClass()
    ?.callStaticMethod<Any>("getUnsafe")
    ?.run { callMethod<Int>("addressSize") }
    ?.also {
      mCurrentAbi = if (it == 8) Abi.ARM64_V8A else Abi.ARMEABI_V7A
      return mCurrentAbi
    }

  // Check so path
  try {
    getApplication().applicationInfo
      .nativeLibraryDir
      .contains("arm64")
      .also { mCurrentAbi = if (it) Abi.ARM64_V8A else Abi.ARMEABI_V7A }
  } catch (e: Throwable) {
    e.printStackTrace()
  }
  return mCurrentAbi
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