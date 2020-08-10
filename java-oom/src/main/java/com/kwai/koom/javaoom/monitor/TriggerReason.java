package com.kwai.koom.javaoom.monitor;

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
public class TriggerReason {

  public enum DumpReason {
    MANUAL_TRIGGER,
    MANUAL_TRIGGER_ON_CRASH,
    HEAP_OVER_THRESHOLD,
    HEAP_THRASHING_HEAVILY,
    HEAP_OOM_CRASH,
    FD_OVER_THRESHOLD,
    FD_OOM_CRASH,
    THREAD_OVER_THRESHOLD,
    THREAD_OOM_CRASH,
  }

  public enum AnalysisReason {
    RIGHT_NOW,
    REANALYSIS,
    TEST
  }

  public DumpReason dumpReason;
  public AnalysisReason analysisReason;

  private static TriggerReason reason;

  private static TriggerReason getTriggerReason() {
    if (reason == null) {
      reason = new TriggerReason();
    }
    return reason;
  }

  public static TriggerReason dumpReason(DumpReason dumpReason) {
    getTriggerReason().dumpReason = dumpReason;
    return reason;
  }

  public static TriggerReason analysisReason(AnalysisReason analysis) {
    getTriggerReason().analysisReason = analysis;
    return reason;
  }
}
