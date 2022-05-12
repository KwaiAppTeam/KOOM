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

import static com.kwai.koom.base.Monitor_ApplicationKt.sdkVersionMatch;
import static com.kwai.koom.base.Monitor_SoKt.loadSoQuietly;

import android.os.Build;

import com.kwai.koom.base.MonitorLog;
import com.kwai.koom.fastdump.ForkJvmHeapDumper;
import com.kwai.koom.fastdump.HeapDumper;

public class ForkStripHeapDumper implements HeapDumper {
  private static final String TAG = "OOMMonitor_ForkStripHeapDumper";
  private boolean mLoadSuccess;

  private static class Holder {
    private static final ForkStripHeapDumper INSTANCE = new ForkStripHeapDumper();
  }

  public static ForkStripHeapDumper getInstance() {
    return ForkStripHeapDumper.Holder.INSTANCE;
  }

  private ForkStripHeapDumper() {}

  private void init() {
    if (mLoadSuccess) {
      return;
    }
    if (loadSoQuietly("koom-strip-dump")) {
      mLoadSuccess = true;
      initStripDump();
    }
  }

  @Override
  public synchronized boolean dump(String path) {
    MonitorLog.i(TAG, "dump " + path);
    if (!sdkVersionMatch()) {
      throw new UnsupportedOperationException("dump failed caused by sdk version not supported!");
    }
    init();
    if (!mLoadSuccess) {
      MonitorLog.e(TAG, "dump failed caused by so not loaded!");
      return false;
    }
    boolean dumpRes = false;
    try {
      hprofName(path);
      dumpRes = ForkJvmHeapDumper.getInstance().dump(path);
      MonitorLog.i(TAG, "dump result " + dumpRes);
    } catch (Exception e) {
      MonitorLog.e(TAG, "dump failed caused by " + e);
      e.printStackTrace();
    }
    return dumpRes;
  }

  public native void initStripDump();

  public native void hprofName(String name);
}