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
 * Created by shenvsv on 2021.
 *
 */

package com.kwai.performance.overhead.thread.monitor

import android.os.Build
import com.google.gson.Gson
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.isArm64
import com.kwai.koom.base.loadSoQuietly
import com.kwai.koom.base.loop.LoopMonitor

object ThreadMonitor : LoopMonitor<ThreadMonitorConfig>() {
  private const val TAG = "koom-thread-monitor"

  @Volatile
  private var mIsRunning = false

  private val mGon by lazy { Gson() }

  fun startTrack() {
    if (handleNativeInit()) {
      mIsRunning = true
      startLoop(clearQueue = true, postAtFront = false, delayMillis = monitorConfig.startDelay)
    }
  }

  fun startTrackAsync() {
    getLoopHandler().postAtFrontOfQueue {
      startTrack()
    }
  }

  fun stop() {
    if (mIsRunning) {
      NativeHandler.stop()
    }
    stopLoop()
  }

  override fun call(): LoopState {
    handleThreadLeak()
    return LoopState.Continue
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  private fun handleThreadLeak() {
    NativeHandler.refresh()
  }

  private fun handleNativeInit(): Boolean {
    if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || Build.VERSION.SDK_INT > Build
            .VERSION_CODES.R) {
      monitorConfig.listener?.onError("not support P below or R above now!")
      return false
    }
    if (!isArm64()) {
      monitorConfig.listener?.onError("support arm64 only!")
      return false
    }
    if (loadSoQuietly("koom-thread")) {
      MonitorLog.i(TAG, "loadLibrary success")
    } else {
      monitorConfig.listener?.onError("loadLibrary fail")
      return false
    }
    if (monitorConfig.disableNativeStack) {
      NativeHandler.disableNativeStack()
    }
    if (monitorConfig.disableJavaStack) {
      NativeHandler.disableJavaStack()
    }
    if (monitorConfig.enableNativeLog) {
      NativeHandler.enableNativeLog()
    }
    NativeHandler.setThreadLeakDelay(monitorConfig.threadLeakDelay)
    NativeHandler.start()
    MonitorLog.i(TAG, "init finish")
    return true
  }

  fun nativeReport(resultJson: String) {
    mGon.fromJson(resultJson, ThreadLeakContainer::class.java).let {
      monitorConfig.listener?.onReport(it.threads)
    }
  }

  fun setListener(listener: ThreadLeakListener) {
    monitorConfig.listener = listener
  }
}
