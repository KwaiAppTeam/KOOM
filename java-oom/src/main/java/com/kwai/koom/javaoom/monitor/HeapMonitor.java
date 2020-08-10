package com.kwai.koom.javaoom.monitor;

import android.util.Log;

import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KGlobalConfig;
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
 * HeapMonitor watch JVM heap running info,
 * and trigger when heap is over threshold
 * several times as HeapThreshold set.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class HeapMonitor implements Monitor {

  private static final String TAG = "HeapMonitor";

  private HeapThreshold heapThreshold;
  private int currentTimes = 0;

  public HeapMonitor() {}

  @Override
  public void setThreshold(Threshold threshold) {
    if (!(threshold instanceof HeapThreshold)) {
      throw new RuntimeException("Must be HeapThreshold!");
    }
    this.heapThreshold = (HeapThreshold) threshold;
  }

  @Override
  public TriggerReason getTriggerReason() {
    return TriggerReason.dumpReason(TriggerReason.DumpReason.HEAP_OVER_THRESHOLD);
  }

  @Override
  public boolean isTrigger() {
    if (!started) {
      return false;
    }

    HeapStatus heapStatus = currentHeapStatus();

    if (heapStatus.isOverThreshold) {
      KLog.i(TAG, "heap status used:" + heapStatus.used / KConstants.Bytes.MB
          + ", max:" + heapStatus.max / KConstants.Bytes.MB
          + ", last over times:" + currentTimes);
      if (heapThreshold.ascending()) {
        if (lastHeapStatus == null || heapStatus.used >= lastHeapStatus.used) {
          currentTimes++;
        } else {
          KLog.i(TAG, "heap status used is not ascending, and over times reset to 0");
          currentTimes = 0;
        }
      } else {
        currentTimes++;
      }
    } else {
      currentTimes = 0;
    }

    lastHeapStatus = heapStatus;
    return currentTimes >= heapThreshold.overTimes();
  }

  private HeapStatus lastHeapStatus;

  private HeapStatus currentHeapStatus() {
    HeapStatus heapStatus = new HeapStatus();
    heapStatus.max = Runtime.getRuntime().maxMemory();
    heapStatus.used = Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory();
    KLog.i(TAG, 100.0f * heapStatus.used / heapStatus.max + " " + heapThreshold.value());
    heapStatus.isOverThreshold = 100.0f * heapStatus.used / heapStatus.max > heapThreshold.value();
    return heapStatus;
  }

  static class HeapStatus {
    long max;
    long used;
    boolean isOverThreshold;
  }

  @Override
  public MonitorType monitorType() {
    return MonitorType.HEAP;
  }

  private volatile boolean started = false;

  @Override
  public void start() {
    started = true;
    if (heapThreshold == null) {
      heapThreshold = KGlobalConfig.getHeapThreshold();
    }
    KLog.i(TAG, "start HeapMonitor, HeapThreshold ratio:" + heapThreshold.value()
        + ", max over times: " + heapThreshold.overTimes());
  }

  @Override
  public void stop() {
    KLog.i(TAG, "stop");
    started = false;
  }

  @Override
  public int pollInterval() {
    return heapThreshold.pollInterval();
  }
}
