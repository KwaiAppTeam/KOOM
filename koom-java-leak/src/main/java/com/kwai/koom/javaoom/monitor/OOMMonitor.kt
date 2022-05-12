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
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import com.kwai.koom.base.*
import com.kwai.koom.base.MonitorManager.getApplication
import com.kwai.koom.base.loop.LoopMonitor
import com.kwai.koom.fastdump.ForkJvmHeapDumper
import com.kwai.koom.javaoom.monitor.OOMFileManager.hprofAnalysisDir
import com.kwai.koom.javaoom.monitor.OOMFileManager.manualDumpDir
import com.kwai.koom.javaoom.monitor.analysis.AnalysisExtraData
import com.kwai.koom.javaoom.monitor.analysis.AnalysisReceiver
import com.kwai.koom.javaoom.monitor.analysis.HeapAnalysisService
import com.kwai.koom.javaoom.monitor.tracker.*
import com.kwai.koom.javaoom.monitor.tracker.model.SystemInfo
import java.io.File
import java.util.*

object OOMMonitor : LoopMonitor<OOMMonitorConfig>(), LifecycleEventObserver {
  private const val TAG = "OOMMonitor"

  private val mOOMTrackers = mutableListOf(
    HeapOOMTracker(), ThreadOOMTracker(), FdOOMTracker(),
    PhysicalMemoryOOMTracker(), FastHugeMemoryOOMTracker()
  )
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
  private var mHasProcessOldHprof = false // Only trigger one time in process running lifecycle.

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
    getLoopHandler().postDelayed({ async { processOldHprofFile() } }, delayMillis)
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
    if (!sdkVersionMatch()) {
      return LoopState.Terminate
    }

    if (mHasDumped) {
      return LoopState.Terminate
    }

    return trackOOM()
  }

  override fun getLoopInterval() = monitorConfig.loopInterval

  private fun isExceedAnalysisTimes(): Boolean {
    MonitorLog.i(
      TAG,
      "OOMPreferenceManager.getAnalysisTimes:${OOMPreferenceManager.getAnalysisTimes()}"
    )

    if (MonitorBuildConfig.DEBUG) {
      return false
    }

    return (OOMPreferenceManager.getAnalysisTimes() > monitorConfig.analysisMaxTimesPerVersion)
      .also { if (it) MonitorLog.e(TAG, "current version is out of max analysis times!") }
  }

  private fun isExceedAnalysisPeriod(): Boolean {
    MonitorLog.i(
      TAG,
      "OOMPreferenceManager.getFirstAnalysisTime():" + OOMPreferenceManager.getFirstLaunchTime()
    )

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

  private fun processOldHprofFile() {
    MonitorLog.i(TAG, "processHprofFile")
    if (mHasProcessOldHprof) {
      return
    }
    mHasProcessOldHprof = true;
    reAnalysisHprof()
    manualDumpHprof()
  }

  private fun reAnalysisHprof() {
    for (file in hprofAnalysisDir.listFiles().orEmpty()) {
      if (!file.exists()) continue

      if (!file.name.startsWith(MonitorBuildConfig.VERSION_NAME)) {
        MonitorLog.i(TAG, "delete other version files ${file.name}")
        file.delete()
        continue
      }

      if (file.canonicalPath.endsWith(".hprof")) {
        val jsonFile = File(file.canonicalPath.replace(".hprof", ".json"))
        if (!jsonFile.exists()) {
          MonitorLog.i(TAG, "create json file and then start service")
          jsonFile.createNewFile()
          startAnalysisService(file, jsonFile, "reanalysis")
        } else {
          MonitorLog.i(
            TAG,
            if (jsonFile.length() == 0L) "last analysis isn't succeed, delete file"
            else "delete old files", true
          )
          jsonFile.delete()
          file.delete()
        }
      }
    }
  }

  private fun manualDumpHprof() {
    for (hprofFile in manualDumpDir.listFiles().orEmpty()) {
      MonitorLog.i(TAG, "manualDumpHprof upload:${hprofFile.absolutePath}")
      monitorConfig.hprofUploader?.upload(hprofFile, OOMHprofUploader.HprofType.STRIPPED)
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
      mForegroundPendingRunnables.add(Runnable {
        startAnalysisService(
          hprofFile,
          jsonFile,
          reason
        )
      })
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

          val content = jsonFile.readText()

          MonitorLogger.addExceptionEvent(content, Logger.ExceptionType.OOM_STACKS)

          monitorConfig.reportUploader?.upload(jsonFile, content)
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

      ForkJvmHeapDumper.getInstance().run {
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
          MonitorLog.i(TAG, "foreground")
          startLoop()
        }

        mForegroundPendingRunnables.forEach { it.run() }
        mForegroundPendingRunnables.clear()
      }
      Lifecycle.Event.ON_STOP -> {
        mIsLoopPendingStart = mIsLoopStarted
        MonitorLog.i(TAG, "background")
        stopLoop()
      }
      else -> Unit
    }
  }
}