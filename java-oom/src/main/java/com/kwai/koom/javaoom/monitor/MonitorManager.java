package com.kwai.koom.javaoom.monitor;

import java.util.ArrayList;
import java.util.List;

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
public class MonitorManager {

  private List<Monitor> monitors;
  private MonitorThread monitorThread;

  public MonitorManager() {
    monitors = new ArrayList<>();
    monitorThread = new MonitorThread();
  }

  public void start() {
    monitorThread.start(monitors);
  }

  public void stop() {
    for (Monitor monitor : monitors) {
      monitor.stop();
    }
    monitorThread.stop();
  }

  public void startMonitor(Monitor monitor) {
    monitor.start();
  }

  public void stopMonitor(Monitor monitor) {
    monitor.stop();
  }

  public void addMonitor(Monitor monitor) {
    monitors.add(monitor);
  }

  public void removeMonitor(Monitor monitor) {
    monitors.remove(monitor);
  }


  public void setTriggerListener(MonitorTriggerListener monitorTriggerListener) {
    monitorThread.setMonitorTriggerListener(monitorTriggerListener);
  }

}
