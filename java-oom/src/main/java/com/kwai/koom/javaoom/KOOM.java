package com.kwai.koom.javaoom;

import android.app.Application;
import android.util.Log;

import com.kwai.koom.javaoom.analysis.HeapAnalysisTrigger;
import com.kwai.koom.javaoom.common.KConfig;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.dump.HeapDumpTrigger;
import com.kwai.koom.javaoom.report.HeapReportUploader;
import com.kwai.koom.javaoom.report.HprofUploader;

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
 * <p>
 * KOOM library entry point.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class KOOM {

  private KOOMInternal internal;
  private static KOOM koom;
  private static boolean inited;
  private static final String TAG = "koom";

  private KOOM() {}
  
  private KOOM(Application application) {
    if (!inited) init(application);
    internal = new KOOMInternal(application);
  }


  /**
   * KOOM entry point, make sure be called in the main thread!
   *
   * @param application application needed
   */
  public static void init(Application application) {
    KLog.init(new KLog.DefaultLogger());

    if (inited) {
      KLog.i(TAG, "already init!");
      return;
    }
    inited = true;

    if (koom == null) {
      koom = new KOOM(application);
    }

    koom.start();
  }

  /**
   * Get KOOM instance.
   *
   * @return koom instance
   */
  public static KOOM getInstance() {
    return koom;
  }

  /**
   * Start KOOM.
   */
  public void start() {
    internal.start();
  }

  /**
   * Stop KOOM.
   */
  public void stop() {
    internal.stop();
  }

  /**
   * Listen KOOM progress status, callback will runs in a separated thread.
   *
   * @param progressListener progressListener
   */
  public void setProgressListener(KOOMProgressListener progressListener) {
    internal.setProgressListener(progressListener);
  }

  /**
   * Get the heap report dir, files are supposed to upload for further memory leak investigation.
   *
   * @return heap report file dir
   */
  public String getReportDir() {
    return internal.getReportDir();
  }

  /**
   * Hprof file are deleted for default, see @setHprofUploader
   *
   * @return heap hprof file dir
   */
  public String getHprofDir() {
    return internal.getHprofDir();
  }

  /**
   * Set custom root dir.
   *
   * @param rootDir root dir
   * @return return false if root dir is invalid.
   */
  public boolean setRootDir(String rootDir) {
    return internal.setRootDir(rootDir);
  }

  /**
   * Set custom config, KOOM will use a default KConfig if custom config is not set.
   *
   * @param kConfig config
   */
  public void setKConfig(KConfig kConfig) {
    internal.setKConfig(kConfig);
  }

  /**
   * Set custom hprof uploader callback if hprof file are needed which are usually
   * not needed in most cases, callback will runs in a separated thread.
   *
   * @param hprofUploader custom hprof uploader
   */
  public void setHprofUploader(HprofUploader hprofUploader) {
    internal.setHprofUploader(hprofUploader);
  }

  /**
   * Set custom heap report file uploader, callback will runs in a separated thread.
   *
   * @param heapReportUploader heap report file uploader
   */
  public void setHeapReportUploader(HeapReportUploader heapReportUploader) {
    internal.setHeapReportUploader(heapReportUploader);
  }


  /**
   * HeapDumpTrigger decides the time of when to dump, override the trigger for advanced purpose.
   *
   * @param heapDumpTrigger heap dump trigger
   */
  public void setHeapDumpTrigger(HeapDumpTrigger heapDumpTrigger) {
    internal.setHeapDumpTrigger(heapDumpTrigger);
  }

  /**
   * HeapAnalysisTrigger decides the time of when to analysis, override the trigger for advanced
   * purpose.
   *
   * @param heapAnalysisTrigger heap analysis trigger
   */
  public void setHeapAnalysisTrigger(HeapAnalysisTrigger heapAnalysisTrigger) {
    internal.setHeapAnalysisTrigger(heapAnalysisTrigger);
  }

  /**
   * Manually trigger heap dump and analysis for usually debug cases.
   * <p>
   * And manually trigger will ignore EnableChecker's limit.
   */
  public void manualTrigger() {
    internal.manualTrigger();
  }

  /**
   * Manually trigger heap dump on crash cases, reanalysis when next launch.
   * <p>
   * Same with 'manualTrigger' this will ignore EnableChecker's limit.
   * <p>
   * Warning: this method may have a high possibility to fail, cause hprof dump cost too much time
   */
  public void manualTriggerOnCrash() {
    internal.manualTriggerOnCrash();
  }

  /***
   * Set a custom logger.
   * 
   * @param logger KLogger's implementation.
   */
  public void setLogger(KLog.KLogger logger) {
    KLog.init(logger);
  }
}
