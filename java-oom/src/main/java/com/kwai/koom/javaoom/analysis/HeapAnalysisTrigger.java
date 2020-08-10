package com.kwai.koom.javaoom.analysis;

import android.app.Application;
import android.util.Log;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.OnLifecycleEvent;

import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.common.KTrigger;
import com.kwai.koom.javaoom.common.KTriggerStrategy;
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
public class HeapAnalysisTrigger implements KTrigger {

  private static final String TAG = "HeapAnalysisTrigger";

  private HeapAnalysisListener heapAnalysisListener;

  public HeapAnalysisTrigger() {}

  public void setHeapAnalysisListener(HeapAnalysisListener heapAnalysisListener) {
    this.heapAnalysisListener = heapAnalysisListener;
  }

  @Override
  public void startTrack() {
    KTriggerStrategy strategy = strategy();
    if (strategy == KTriggerStrategy.RIGHT_NOW) {
      trigger(TriggerReason.analysisReason(TriggerReason.AnalysisReason.RIGHT_NOW));
    }
  }

  @Override
  public void stopTrack() {}

  public void doAnalysis(Application application) {
    HeapAnalyzeService.runAnalysis(application, heapAnalysisListener);
  }

  private boolean triggered;

  @Override
  public void trigger(TriggerReason triggerReason) {
    //do trigger when foreground
    if (!isForeground) {
      KLog.i(TAG, "reTrigger when foreground");
      this.reTriggerReason = triggerReason;
      return;
    }

    KLog.i(TAG, "trigger reason:" + triggerReason.analysisReason);

    if (triggered) {
      KLog.i(TAG, "Only once trigger!");
      return;
    }
    triggered = true;

    HeapAnalyzeReporter.addAnalysisReason(triggerReason.analysisReason);

    if (triggerReason.analysisReason == TriggerReason.AnalysisReason.REANALYSIS) {
      HeapAnalyzeReporter.recordReanalysis();
    }

    //test reanalysis
    //if (triggerReason.analysisReason != TriggerReason.AnalysisReason.REANALYSIS) return;

    if (heapAnalysisListener != null) {
      heapAnalysisListener.onHeapAnalysisTrigger();
    }

    doAnalysis(KGlobalConfig.getApplication());
  }

  private KTriggerStrategy strategy;

  public void setStrategy(KTriggerStrategy strategy) {
    this.strategy = strategy;
  }

  @Override
  public KTriggerStrategy strategy() {
    if (strategy != null) {
      return strategy;
    }

    return KTriggerStrategy.RIGHT_NOW;
  }

  private volatile boolean isForeground;
  private TriggerReason reTriggerReason;

  @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
  public void onBackground() {
    KLog.i(TAG, "onBackground");
    isForeground = false;
  }

  @OnLifecycleEvent(Lifecycle.Event.ON_START)
  public void onForeground() {
    KLog.i(TAG, "onForeground");
    isForeground = true;
    if (reTriggerReason != null) {
      TriggerReason tmpReason = reTriggerReason;
      reTriggerReason = null;
      trigger(tmpReason);
    }
  }
}
