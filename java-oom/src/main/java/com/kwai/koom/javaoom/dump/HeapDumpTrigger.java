package com.kwai.koom.javaoom.dump;

import android.util.Log;

import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.common.KTrigger;
import com.kwai.koom.javaoom.common.KTriggerStrategy;
import com.kwai.koom.javaoom.common.KVData;
import com.kwai.koom.javaoom.monitor.HeapMonitor;
import com.kwai.koom.javaoom.monitor.MonitorManager;
import com.kwai.koom.javaoom.monitor.TriggerReason;
import com.kwai.koom.javaoom.report.HeapAnalyzeReporter;

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
public class HeapDumpTrigger implements KTrigger {

  private static final String TAG = "HeapDumpTrigger";

  private MonitorManager monitorManager;
  private HeapDumper heapDumper;

  public HeapDumpTrigger() {
    monitorManager = new MonitorManager();
    monitorManager.addMonitor(new HeapMonitor());
    heapDumper = new ForkJvmHeapDumper();
  }

  public void setHeapDumper(HeapDumper heapDumper) {
    this.heapDumper = heapDumper;
  }

  @Override
  public void startTrack() {
    monitorManager.start();
    monitorManager.setTriggerListener((monitorType, reason) -> {
      trigger(reason);
      return true;
    });
  }

  @Override
  public void stopTrack() {
    monitorManager.stop();
  }

  public void doHeapDump(TriggerReason.DumpReason reason) {
    KLog.i(TAG, "doHeapDump");

    KHeapFile.getKHeapFile().buildFiles();

    HeapAnalyzeReporter.addDumpReason(reason);
    HeapAnalyzeReporter.addDeviceRunningInfo();

    boolean res = heapDumper.dump(KHeapFile.getKHeapFile().hprof.path);

    if (res) {
      heapDumpListener.onHeapDumped(reason);
    } else {
      KLog.e(TAG, "heap dump failed!");
      heapDumpListener.onHeapDumpFailed();
      KHeapFile.delete();
    }
  }

  @Override
  public KTriggerStrategy strategy() {
    return KTriggerStrategy.RIGHT_NOW;
  }

  private boolean triggered;

  @Override
  public void trigger(TriggerReason reason) {
    if (triggered) {
      KLog.e(TAG, "Only once trigger!");
      return;
    }
    triggered = true;

    monitorManager.stop();

    KLog.i(TAG, "trigger reason:" + reason.dumpReason);
    if (heapDumpListener != null) {
      heapDumpListener.onHeapDumpTrigger(reason.dumpReason);
    }

    try {
      doHeapDump(reason.dumpReason);
    } catch (Exception e) {
      KLog.e(TAG, "doHeapDump failed");
      e.printStackTrace();
      if (heapDumpListener != null) {
        heapDumpListener.onHeapDumpFailed();
      }
    }

    KVData.addTriggerTime(KGlobalConfig.getRunningInfoFetcher().appVersion());
  }

  private HeapDumpListener heapDumpListener;

  public void setHeapDumpListener(HeapDumpListener heapDumpListener) {
    this.heapDumpListener = heapDumpListener;
  }

  @Override
  public void onForeground() {
    //do noting now
  }

  @Override
  public void onBackground() {
    //do nothing now
  }
}
