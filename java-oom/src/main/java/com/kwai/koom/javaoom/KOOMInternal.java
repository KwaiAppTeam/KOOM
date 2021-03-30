package com.kwai.koom.javaoom;

import android.app.Application;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import androidx.lifecycle.ProcessLifecycleOwner;

import com.kwai.koom.javaoom.analysis.HeapAnalysisListener;
import com.kwai.koom.javaoom.analysis.HeapAnalysisTrigger;
import com.kwai.koom.javaoom.analysis.ReanalysisChecker;
import com.kwai.koom.javaoom.common.KConfig;
import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.common.KSoLoader;
import com.kwai.koom.javaoom.common.KUtils;
import com.kwai.koom.javaoom.dump.HeapDumpListener;
import com.kwai.koom.javaoom.dump.HeapDumpTrigger;
import com.kwai.koom.javaoom.monitor.TriggerReason;
import com.kwai.koom.javaoom.report.HeapReportUploader;
import com.kwai.koom.javaoom.report.HprofUploader;

import java.io.File;

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
class KOOMInternal implements HeapDumpListener, HeapAnalysisListener {

  private static final String TAG = "KOOM";

  private HeapDumpTrigger heapDumpTrigger;
  private HeapAnalysisTrigger heapAnalysisTrigger;

  private KOOMProgressListener kProgressListener;

  private KOOMInternal() {}

  public KOOMInternal(Application application) {
    KUtils.startup();

    buildConfig(application);

    heapDumpTrigger = new HeapDumpTrigger();
    heapAnalysisTrigger = new HeapAnalysisTrigger();

    ProcessLifecycleOwner.get().getLifecycle().addObserver(heapAnalysisTrigger);
  }

  private void buildConfig(Application application) {
    //setApplication must be the first
    KGlobalConfig.setApplication(application);
    KGlobalConfig.setKConfig(KConfig.defaultConfig());
  }

  public void setKConfig(KConfig kConfig) {
    KGlobalConfig.setKConfig(kConfig);
  }

  private Handler koomHandler;

  public void start() {
    HandlerThread koomThread = new HandlerThread("koom");
    koomThread.start();
    koomHandler = new Handler(koomThread.getLooper());
    startInKOOMThread();
  }

  private void startInKOOMThread() {
    koomHandler.postDelayed(this::startInternal, KConstants.Perf.START_DELAY);
  }

  private boolean started;

  private void startInternal() {
    try {
      if (started) {
        KLog.i(TAG, "already started!");
        return;
      }
      started = true;

      heapDumpTrigger.setHeapDumpListener(this);
      heapAnalysisTrigger.setHeapAnalysisListener(this);

      if (KOOMEnableChecker.doCheck() != KOOMEnableChecker.Result.NORMAL) {
        KLog.e(TAG, "koom start failed, check result: " + KOOMEnableChecker.doCheck());
        return;
      }

      ReanalysisChecker reanalysisChecker = new ReanalysisChecker();
      if (reanalysisChecker.detectReanalysisFile() != null) {
        KLog.i(TAG, "detected reanalysis file");
        heapAnalysisTrigger
            .trigger(TriggerReason.analysisReason(TriggerReason.AnalysisReason.REANALYSIS));
        return;
      }

      heapDumpTrigger.startTrack();
    } catch (Exception e) {
      e.printStackTrace();
    }
  }

  public void stop() {
    if (heapDumpTrigger != null) {
      heapDumpTrigger.stopTrack();
    }
    if (heapAnalysisTrigger != null) {
      heapAnalysisTrigger.stopTrack();
    }
  }

  public void setSoLoader(KSoLoader soLoader) {
    KGlobalConfig.setSoLoader(soLoader);
  }

  public boolean setRootDir(String rootDir) {
    File dir = new File(rootDir);
    if (!dir.exists()) {
      return false;
    }
    KGlobalConfig.setRootDir(rootDir);
    return true;
  }

  public String getReportDir() {
    return KGlobalConfig.getReportDir();
  }

  public String getHprofDir() {
    return KGlobalConfig.getHprofDir();
  }

  public void setHeapDumpTrigger(HeapDumpTrigger heapDumpTrigger) {
    this.heapDumpTrigger = heapDumpTrigger;
  }

  public void setHeapAnalysisTrigger(HeapAnalysisTrigger heapAnalysisTrigger) {
    this.heapAnalysisTrigger = heapAnalysisTrigger;
  }

  public void setProgressListener(KOOMProgressListener progressListener) {
    this.kProgressListener = progressListener;
  }

  public void changeProgress(KOOMProgressListener.Progress progress) {
    if (kProgressListener != null) {
      kProgressListener.onProgress(progress);
    }
  }

  @Override
  public void onHeapDumpTrigger(TriggerReason.DumpReason reason) {
    KLog.i(TAG, "onHeapDumpTrigger");
    changeProgress(KOOMProgressListener.Progress.HEAP_DUMP_START);
  }

  @Override
  public void onHeapDumped(TriggerReason.DumpReason reason) {
    KLog.i(TAG, "onHeapDumped");
    changeProgress(KOOMProgressListener.Progress.HEAP_DUMPED);

    //Crash cases need to reanalyze next launch and not do analyze right now.
    if (reason != TriggerReason.DumpReason.MANUAL_TRIGGER_ON_CRASH) {
      heapAnalysisTrigger.startTrack();
    } else {
      KLog.i(TAG, "reanalysis next launch when trigger on crash");
    }
  }

  @Override
  public void onHeapDumpFailed() {
    changeProgress(KOOMProgressListener.Progress.HEAP_DUMP_FAILED);
  }

  @Override
  public void onHeapAnalysisTrigger() {
    KLog.i(TAG, "onHeapAnalysisTrigger");
    changeProgress(KOOMProgressListener.Progress.HEAP_ANALYSIS_START);
  }

  @Override
  public void onHeapAnalyzed() {
    KLog.i(TAG, "onHeapAnalyzed");
    changeProgress(KOOMProgressListener.Progress.HEAP_ANALYSIS_DONE);
    uploadFiles(KHeapFile.getKHeapFile());
  }

  @Override
  public void onHeapAnalyzeFailed() {
    changeProgress(KOOMProgressListener.Progress.HEAP_ANALYSIS_FAILED);
  }

  private void uploadFiles(KHeapFile heapFile) {
    uploadHprof(heapFile.hprof);
    uploadHeapReport(heapFile.report);
  }

  private void uploadHprof(KHeapFile.Hprof hprof) {
    if (hprofUploader != null) {
      hprofUploader.upload(hprof.file());
    }
    //Do not save the hprof file by default.
    if (hprofUploader == null || hprofUploader.deleteWhenUploaded()) {
      KLog.i(TAG, "delete " + hprof.path);
      hprof.delete();
    }
  }

  private void uploadHeapReport(KHeapFile.Report report) {
    if (heapReportUploader != null) {
      heapReportUploader.upload(report.file());
    }
    //Save the report file by default.
    if (heapReportUploader != null && heapReportUploader.deleteWhenUploaded()) {
      KLog.i(TAG, "report delete");
      report.delete();
    }
  }

  private HprofUploader hprofUploader;
  private HeapReportUploader heapReportUploader;

  public void setHprofUploader(HprofUploader hprofUploader) {
    this.hprofUploader = hprofUploader;
  }

  public void setHeapReportUploader(HeapReportUploader heapReportUploader) {
    this.heapReportUploader = heapReportUploader;
  }

  private void manualTriggerInternal() {
    if (!started) {
      startInternal();
    }
    if (started) {
      heapDumpTrigger.trigger(TriggerReason.dumpReason(
          TriggerReason.DumpReason.MANUAL_TRIGGER));
    }
  }

  public void manualTrigger() {
    koomHandler.post(this::manualTriggerInternal);
  }


  private void manualTriggerOnCrashInternal() {
    if (!started) {
      startInternal();
    }
    if (started) {
      heapDumpTrigger.trigger(TriggerReason.dumpReason(
          TriggerReason.DumpReason.MANUAL_TRIGGER_ON_CRASH));
    }
  }

  public void manualTriggerOnCrash() {
    koomHandler.post(this::manualTriggerOnCrashInternal);
  }
}
