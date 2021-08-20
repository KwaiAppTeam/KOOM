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

package com.kwai.koom.javaoom.hprof;

import com.kwai.koom.base.MonitorLog;

public class ForkStripHeapDumper extends HeapDumper {
  private static final String TAG = "OOMMonitor_ForkStripHeapDumper";

  public ForkStripHeapDumper() {
    super();
  }

  @Override
  public boolean dump(String path) {
    MonitorLog.e(TAG, "dump " + path);
    boolean dumpRes;
    try {
      StripHprofHeapDumper stripHprofHeapDumper = new StripHprofHeapDumper();
      stripHprofHeapDumper.initStripDump();
      stripHprofHeapDumper.hprofName(path);

      ForkJvmHeapDumper forkJvmHeapDumper = new ForkJvmHeapDumper();
      dumpRes = forkJvmHeapDumper.dump(path);
      MonitorLog.e(TAG, "dump end");
    } catch (Exception e) {
      e.printStackTrace();
      return false;
    }
    return dumpRes;
  }
}
