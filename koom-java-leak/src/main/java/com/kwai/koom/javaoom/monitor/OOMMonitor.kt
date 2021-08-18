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

package com.kwai.koom.javaoom.monitor

import android.os.Build
import android.os.SystemClock

import java.io.File
import java.util.*

import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner

import com.kwai.koom.base.CommonConfig
import com.kwai.koom.base.Logger
import com.kwai.koom.base.MonitorBuildConfig
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.MonitorLogger
import com.kwai.koom.base.MonitorManager.getApplication
import com.kwai.koom.base.async
import com.kwai.koom.base.currentActivity
import com.kwai.koom.base.isForeground
import com.kwai.koom.base.isMainProcess
import com.kwai.koom.base.loop.LoopMonitor
import com.kwai.koom.base.registerProcessLifecycleObserver
import com.kwai.koom.javaoom.hprof.ForkJvmHeapDumper
import com.kwai.koom.javaoom.monitor.OOMFileManager.hprofAnalysisDir
import com.kwai.koom.javaoom.monitor.OOMFileManager.oomDumDir
import com.kwai.koom.javaoom.monitor.analysis.AnalysisExtraData
import com.kwai.koom.javaoom.monitor.analysis.AnalysisReceiver
import com.kwai.koom.javaoom.monitor.analysis.HeapAnalysisService
import com.kwai.koom.javaoom.monitor.tracker.FastHugeMemoryOOMTracker
import com.kwai.koom.javaoom.monitor.tracker.FdOOMTracker
import com.kwai.koom.javaoom.monitor.tracker.HeapOOMTracker
import com.kwai.koom.javaoom.monitor.tracker.PhysicalMemoryOOMTracker
import com.kwai.koom.javaoom.monitor.tracker.ThreadOOMTracker
import com.kwai.koom.javaoom.monitor.tracker.model.SystemInfo

object OOMMonitor : LoopMonitor<OOMMonitorConfig>(), LifecycleEventObserver {
  private const val TAG = "OOMMonitor"

  private val mOOMTrackers = mutableListOf(
      HeapOOMTracker(), ThreadOOMTracker(), FdOOMTracker(),
      PhysicalMemoryOOMTracker(), FastHugeMemoryOOMTracker())
  private val mTrackReasons = mutableListOf<String>()

  private var mMonitorInitTime = 0L

  private var mForegroundPendingRunnables = mutableListOf<Runnable>()

  @Volatile
  private var mIsLoopStarted = false

  @Volatile
  private var mIsLoopPendingStart = false

  @Volatile
  private var mHasDumped = false // Only trigger one time in process running lifecycle.

  @Volatile
  private var mHasAnalysedLatestHprof = false // Only trigger one time in process running lifecycle.

  override fun init(commonConfig: CommonConfig, monitorConfig: OOMMonitorConfig) {
    super.init(commonConfig, monitorConfig)

    mMonitorInitTime = SystemClock.elapsedRealtime()

    OOMPreferenceManager.init(commonConfig.sharedPreferencesInvoker)
    OOMFileManager.init(commonConfig.rootFileInvoker)

    for (oomTracker in mOOMTrackers) {
      oomTracker.init(commonConfig, monitorConfig)
    }

    getApplication().registerProcessLifecycleObserver(this)
  }

  override fun startLoop(clearQueue: Boolean, postAtFront: Boolean, delayMillis: Long) {
    throwIfNotInitialized { return }

    if (!isMainProcess()) {
      return
    }

    MonitorLog.i(TAG, "startLoop()")

    if (mIsLoopStarted) {
      return
    }
    mIsLoopStarted = true

    super.startLoop(clearQueue, postAtFront, delayMillis)
    getLoopHandler().postDelayed({ async { analysisLatestHprofFile() } }, delayMillis)
  }

  override fun stopLoop() {
    throwIfNotInitialized { return }

    if (!isMainProcess()) {
      return
    }

    super.stopLoop()

    MonitorLog.i(TAG, "stopLoop()")

    mIsLoopStarted = false
  }

  override fun call(): LoopState {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP
        || Build.VERSION.SDK_INT > Build.VERSION_CODES.R) {
      return LoopState.Terminate
    }

    if (mHasDumped) {
      return LoopState.Terminate
    }

    return trackOOM()
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  private fun isExceedAnalysisTimes(): Boolean {
    MonitorLog.i(TAG, "OOMPreferenceManager.getAnalysisTimes:${OOMPreferenceManager.getAnalysisTimes()}")

    if (MonitorBuildConfig.DEBUG) {
      return false
    }

    return (OOMPreferenceManager.getAnalysisTimes() > monitorConfig.analysisMaxTimesPerVersion)
        .also { if (it) MonitorLog.e(TAG, "current version is out of max analysis times!") }
  }

  private fun isExceedAnalysisPeriod(): Boolean {
    MonitorLog.i(TAG, "OOMPreferenceManager.getFirstAnalysisTime():" + OOMPreferenceManager.getFirstLaunchTime())

    if (MonitorBuildConfig.DEBUG) {
      return false
    }

    val analysisPeriod = System.currentTimeMillis() - OOMPreferenceManager.getFirstLaunchTime()

    return (analysisPeriod > monitorConfig.analysisPeriodPerVersion)
        .also { if (it) MonitorLog.e(TAG, "current version is out of max analysis period!") }
  }

  private fun trackOOM(): LoopState {
    SystemInfo.refresh()

    mTrackReasons.clear()
    for (oomTracker in mOOMTrackers) {
      if (oomTracker.track()) {
        mTrackReasons.add(oomTracker.reason())
      }
    }

    if (mTrackReasons.isNotEmpty() && monitorConfig.enableHprofDumpAnalysis) {
      if (isExceedAnalysisPeriod() || isExceedAnalysisTimes()) {
        MonitorLog.e(TAG, "Triggered, but exceed analysis times or period!")
      } else {
        async {
          MonitorLog.i(TAG, "mTrackReasons:${mTrackReasons}")
          dumpAndAnalysis()
        }
      }

      return LoopState.Terminate
    }

    return LoopState.Continue
  }

  /**
   * 如果存在，压缩上传或重新触发本地内存镜像分析的相关文件
   * 具体：
   * 假设有文件只有hprof，无json文件，则触发且只触发一次重新分析
   * 假设文件有hprof和完整的json，则表明上次没有成功上传，重新上传
   */
  private fun analysisLatestHprofFile() {
    try {
      if (mHasAnalysedLatestHprof) {
        return
      }
      MonitorLog.i(TAG, "analysisLatestHprofFile")
      mHasAnalysedLatestHprof = true

      for (hprofFile in hprofAnalysisDir.listFiles().orEmpty()) {
        if (!hprofFile.exists()) continue

        if (!hprofFile.name.startsWith(MonitorBuildConfig.VERSION_NAME)) {
          MonitorLog.i(TAG, "delete other version files")
          hprofFile.delete()
          continue
        }

        if (hprofFile.canonicalPath.endsWith(".hprof")) {
          val jsonFile = File(hprofFile.canonicalPath.replace(".hprof", ".json"))

          if (!jsonFile.exists()) {
            MonitorLog.i(TAG, "retry analysis, json not exist, then start service")

            // 创建json file，表示只触发1次，后续分析失败也不再继续分析
            jsonFile.createNewFile()

            startAnalysisService(hprofFile, jsonFile, "reanalysis")
          } else if (jsonFile.length() == 0L) {
            MonitorLog.i(TAG, "retry analysis, json file exists but length 0, this means " +
                "koom didn't success in last analysis, so delete the files", true)

            // 表示是重新触发过1次，依然失败的案例，直接将其删除避免后续重复分析
            jsonFile.delete()
            hprofFile.delete()
          } else {
            MonitorLog.i(TAG, "retry analysis, json file length normal, this means it is" +
                " success in last analysis, delete hprof and json files")

            //有待观察是否需要上传
            jsonFile.delete()
            hprofFile.delete()
          }
        }
      }

      for (hprofFile in oomDumDir.listFiles().orEmpty()) {
        MonitorLog.i(TAG, "OOM Dump upload:${hprofFile.absolutePath}")
        // TODO HPROF来源
        monitorConfig.hprofUploader?.upload(hprofFile, OOMHprofUploader.HprofType.STRIPPED)
      }

    } catch (e: Exception) {
      e.printStackTrace()
      MonitorLog.e(TAG, "retryAnalysisFailed: " + e.message, true)
    }
  }

  private fun startAnalysisService(
      hprofFile: File,
      jsonFile: File,
      reason: String
  ) {
    if (hprofFile.length() == 0L) {
      hprofFile.delete()
      MonitorLog.i(TAG, "hprof file size 0", true)
      return
    }

    if (!getApplication().isForeground) {
      MonitorLog.e(TAG, "try startAnalysisService, but not foreground")
      mForegroundPendingRunnables.add(Runnable { startAnalysisService(hprofFile, jsonFile, reason) })
      return
    }

    OOMPreferenceManager.increaseAnalysisTimes()

    val extraData = AnalysisExtraData().apply {
      this.reason = reason
      this.currentPage = getApplication().currentActivity?.localClassName.orEmpty()
      this.usageSeconds = "${(SystemClock.elapsedRealtime() - mMonitorInitTime) / 1000}"
    }

    HeapAnalysisService.startAnalysisService(
        getApplication(),
        hprofFile.canonicalPath,
        jsonFile.canonicalPath,
        extraData,
        object : AnalysisReceiver.ResultCallBack {
          override fun onError() {
            MonitorLog.e(TAG, "heap analysis error, do file delete", true)

            hprofFile.delete()
            jsonFile.delete()
          }

          override fun onSuccess() {
            MonitorLog.i(TAG, "heap analysis success, do upload", true)

            MonitorLogger.addExceptionEvent(jsonFile.readText(), Logger.ExceptionType.OOM_STACKS)

            monitorConfig.hprofUploader?.upload(hprofFile, OOMHprofUploader.HprofType.ORIGIN)
          }
        })
  }

  private fun dumpAndAnalysis() {
    MonitorLog.i(TAG, "dumpAndAnalysis");
    runCatching {
      if (!OOMFileManager.isSpaceEnough()) {
        MonitorLog.e(TAG, "available space not enough", true)
        return@runCatching
      }
      if (mHasDumped) {
        return
      }
      mHasDumped = true

      val date = Date()

      val jsonFile = OOMFileManager.createJsonAnalysisFile(date)
      val hprofFile = OOMFileManager.createHprofAnalysisFile(date).apply {
        createNewFile()
        setWritable(true)
        setReadable(true)
      }

      MonitorLog.i(TAG, "hprof analysis dir:$hprofAnalysisDir")

      ForkJvmHeapDumper().run {
        dump(hprofFile.absolutePath)
      }

      MonitorLog.i(TAG, "end hprof dump", true)
      Thread.sleep(1000) // make sure file synced to disk.
      MonitorLog.i(TAG, "start hprof analysis")

      startAnalysisService(hprofFile, jsonFile, mTrackReasons.joinToString())
    }.onFailure {
      it.printStackTrace()

      MonitorLog.i(TAG, "onJvmThreshold Exception " + it.message, true)
    }
  }

  override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
    when (event) {
      Lifecycle.Event.ON_START -> {
        if (!mHasDumped && mIsLoopPendingStart) {
          startLoop()
        }

        mForegroundPendingRunnables.forEach { it.run() }
        mForegroundPendingRunnables.clear()
      }
      Lifecycle.Event.ON_STOP -> {
        mIsLoopPendingStart = mIsLoopStarted

        stopLoop()
      }
      else -> Unit
    }
  }
}