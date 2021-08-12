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

package com.kwai.koom.javaoom.hprof;

import android.os.Build;
import android.os.Debug;

import java.io.IOException;

import com.kwai.koom.base.MonitorLog;

public class ForkJvmHeapDumper extends HeapDumper {

  private static final String TAG = "ForkJvmHeapDumper";

  public ForkJvmHeapDumper() {
    super();
    if (soLoaded) {
      initForkDump();
    }
  }

  @Override
  public boolean dump(String path) {
    if (!soLoaded) {
      MonitorLog.e(TAG, "dump failed caused by so not loaded!");
      return false;
    }

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP
      || Build.VERSION.SDK_INT > Build.VERSION_CODES.R) {
      MonitorLog.e(TAG, "dump failed caused by version net permitted!");
      return false;
    }

    // Compatible with Android 11
    if (Build.VERSION.SDK_INT == Build.VERSION_CODES.R) {
      return dumpHprofDataNative(path);
    }

    //todo 增加磁盘校验

    boolean dumpRes = false;
    try {
      int pid = trySuspendVMThenFork();
      if (pid == 0) {
        Debug.dumpHprofData(path);
        MonitorLog.i(TAG, "notifyDumped:" + dumpRes);
        exitProcess();
      } else {
        resumeVM();
        dumpRes = waitDumping(pid);
        MonitorLog.i(TAG, "hprof pid:" + pid + " dumped: " + path);
      }

    } catch (IOException e) {
      e.printStackTrace();
      MonitorLog.e(TAG, "dump failed caused by IOException!");
    }
    return dumpRes;
  }

  private boolean waitDumping(int pid) {
    waitPid(pid);
    return true;
  }

  /**
   * Init before do dump.
   *
   * @return init result
   */
  private native void initForkDump();

  /**
   * First do suspend vm, then do fork.
   *
   * @return result of fork
   */
  private native int trySuspendVMThenFork();

  /**
   * Wait process exit.
   *
   * @param pid waited process.
   */
  private native void waitPid(int pid);

  /**
   * Exit current process.
   */
  private native void exitProcess();

  /**
   * Resume the VM.
   */
  private native void resumeVM();

  /**
   * Dump hprof with hidden c++ API
   */
  public static native boolean dumpHprofDataNative(String fileName);
}
