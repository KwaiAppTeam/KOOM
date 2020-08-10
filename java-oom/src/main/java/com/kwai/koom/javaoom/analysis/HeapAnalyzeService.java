package com.kwai.koom.javaoom.analysis;

import android.app.Application;
import android.app.IntentService;
import android.content.Intent;
import android.os.ResultReceiver;
import android.util.Log;

import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;

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
 * Heap analysis runs in a separated process by using IntentService.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class HeapAnalyzeService extends IntentService {

  private static final String TAG = "HeapAnalyzeService";

  public HeapAnalyzeService() {
    super("HeapAnalyzeService");
  }

  /**
   * Creates an IntentService.  Invoked by your subclass's constructor.
   *
   * @param name Used to name the worker thread, important only for debugging.
   */
  public HeapAnalyzeService(String name) {
    super(name);
  }

  public static void runAnalysis(Application application,
      HeapAnalysisListener heapAnalysisListener) {
    KLog.i(TAG, "runAnalysis startService");
    Intent intent = new Intent(application, HeapAnalyzeService.class);
    IPCReceiver ipcReceiver = buildAnalysisReceiver(heapAnalysisListener);
    intent.putExtra(KConstants.ServiceIntent.RECEIVER, ipcReceiver);
    KHeapFile heapFile = KHeapFile.getKHeapFile();
    intent.putExtra(KConstants.ServiceIntent.HEAP_FILE, heapFile);
    application.startService(intent);
  }

  private static IPCReceiver buildAnalysisReceiver(HeapAnalysisListener heapAnalysisListener) {
    return new IPCReceiver(new IPCReceiver.ReceiverCallback() {
      @Override
      public void onSuccess() {
        KLog.i(TAG, "IPC call back, heap analysis success");
        heapAnalysisListener.onHeapAnalyzed();
      }

      @Override
      public void onError() {
        KLog.i(TAG, "IPC call back, heap analysis failed");
        heapAnalysisListener.onHeapAnalyzeFailed();
      }
    });
  }

  private ResultReceiver ipcReceiver;
  private KHeapAnalyzer heapAnalyzer;

  @Override
  protected void onHandleIntent(Intent intent) {
    KLog.i(TAG, "start analyze pid:" + android.os.Process.myPid());
    boolean res = false;
    try {
      beforeAnalyze(intent);
      res = doAnalyze();
    } catch (Throwable e) {
      e.printStackTrace();
    }
    if (ipcReceiver != null) {
      ipcReceiver.send(res ? IPCReceiver.RESULT_CODE_OK
          : IPCReceiver.RESULT_CODE_FAIL, null);
    }
  }

  /**
   * run in the heap_analysis process
   *
   * @param intent intent contains device running meta info from main process
   */
  private void beforeAnalyze(Intent intent) {
    assert intent != null;
    ipcReceiver = intent.getParcelableExtra(KConstants.ServiceIntent.RECEIVER);
    KHeapFile heapFile = intent.getParcelableExtra(KConstants.ServiceIntent.HEAP_FILE);
    KHeapFile.buildInstance(heapFile);
    assert heapFile != null;
    heapAnalyzer = new KHeapAnalyzer(heapFile);
  }

  /**
   * run in the heap_analysis process
   */
  private boolean doAnalyze() {
    return heapAnalyzer.analyze();
  }
}
