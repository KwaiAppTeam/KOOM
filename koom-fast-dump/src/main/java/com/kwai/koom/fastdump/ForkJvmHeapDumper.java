/*
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
 * A jvm hprof dumper which use fork and don't block main process.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.fastdump;

import static com.kwai.koom.base.Monitor_ApplicationKt.sdkVersionMatch;
import static com.kwai.koom.base.Monitor_SoKt.loadSoQuietly;

import android.text.TextUtils;

import androidx.annotation.NonNull;

import com.kwai.koom.base.MonitorLog;

public class ForkJvmHeapDumper implements HeapDumper {
  private static final String TAG = "OOMMonitor_ForkJvmHeapDumper";
  private boolean mLoadSuccess;

  private static class Holder {
    private static final ForkJvmHeapDumper INSTANCE = new ForkJvmHeapDumper();
  }

  public static ForkJvmHeapDumper getInstance() {
    return ForkJvmHeapDumper.Holder.INSTANCE;
  }

  private ForkJvmHeapDumper() {}

  private void init () {
    if (mLoadSuccess) {
      return;
    }
    if (loadSoQuietly("koom-fast-dump")) {
      mLoadSuccess = true;
      nativeInit();
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

    if (TextUtils.isEmpty(path)) {
      MonitorLog.e(TAG, "dump failed caused by empty path!");
      return false;
    }

    boolean dumpRes = forkDump(path, true);
    MonitorLog.i(TAG, String.format("dump to %s %s %s", path, "and wait", dumpRes ? "success" : "failure"));
    return dumpRes;
  }

  /**
   * Init before do dump.
   */
  private native void nativeInit();

  private native boolean forkDump(@NonNull String path, boolean waitPid);
}
