package com.kwai.koom.javaoom.monitor;

import java.util.ArrayList;
import java.util.List;

import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import com.kwai.koom.javaoom.common.KConstants;

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
public class MonitorThread {

  private static final String TAG = "MonitorThread";

  private HandlerThread thread;
  private Handler handler;

  public MonitorThread() {
    thread = new HandlerThread("MonitorThread");
    thread.start();
    handler = new Handler(thread.getLooper());
  }

  public void start(List<Monitor> monitors) {
    stop = false;

    Log.i(TAG, "start");

    List<Runnable> runnables = new ArrayList<>();
    for (Monitor monitor : monitors) {
      monitor.start();
      runnables.add(new MonitorRunnable(monitor));
    }
    for (Runnable runnable : runnables) {
      handler.post(runnable);
    }
  }

  public void stop() {
    stop = true;
  }

  private MonitorTriggerListener monitorTriggerListener;

  public void setMonitorTriggerListener(MonitorTriggerListener monitorTriggerListener) {
    this.monitorTriggerListener = monitorTriggerListener;
  }

  private volatile boolean stop = false;

  class MonitorRunnable implements Runnable {

    private Monitor monitor;

    public MonitorRunnable(Monitor monitor) {
      this.monitor = monitor;
    }

    @Override
    public void run() {
      if (stop) {
        return;
      }

      if (KConstants.Debug.VERBOSE_LOG) {
        Log.i(TAG, monitor.monitorType() + " monitor run");
      }

      if (monitor.isTrigger()) {
        Log.i(TAG, monitor.monitorType() + " monitor "
            + monitor.monitorType() + " trigger");
        stop = monitorTriggerListener
            .onTrigger(monitor.monitorType(), monitor.getTriggerReason());
      }

      if (!stop) {
        handler.postDelayed(this, monitor.pollInterval());
      }
    }
  }
}
