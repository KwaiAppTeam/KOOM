package com.kwai.koom.javaoom.dump;

import java.io.IOException;

import android.os.Build;
import android.os.Debug;

import com.kwai.koom.javaoom.KOOMEnableChecker;
import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KLog;

import java.io.File;

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
 * A jvm hprof dumper which use fork and don't block main process.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class ForkJvmHeapDumper implements HeapDumper {

  private static final String TAG = "ForkJvmHeapDumper";

  private boolean soLoaded;

  public ForkJvmHeapDumper() {
    soLoaded = KGlobalConfig.getSoLoader().loadLib("koom-java");
    if (soLoaded) {
      initForkDump(Build.VERSION.SDK_INT);
    }
  }

  @Override
  public boolean dump(String path) {
    KLog.i(TAG, "dump " + path);
    if (!soLoaded) {
      KLog.e(TAG, "dump failed caused by so not loaded!");
      return false;
    }

    if (!KOOMEnableChecker.get().isVersionPermit()) {
      KLog.e(TAG, "dump failed caused by version net permitted!");
      return false;
    }

    if (!KOOMEnableChecker.get().isSpaceEnough()) {
      KLog.e(TAG, "dump failed caused by disk space not enough!");
      return false;
    }

    //modify file permission, adapt to some rom
    File file = new File(path);
    if (file.exists()) {
      file.setReadable(true, false);
    } else {
      try {
        file.createNewFile();
        file.setReadable(true, false);
      } catch (IOException e) {
        e.printStackTrace();
      }
    }

    // Compatible with Android 11
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.Q) {
      return dumpHprofDataNative(path);
    }

    boolean dumpRes = false;
    try {
      int pid = trySuspendVMThenFork();
      if (pid == 0) {
        Debug.dumpHprofData(path);
        KLog.i(TAG, "notifyDumped:" + dumpRes);
        //System.exit(0);
        exitProcess();
      } else {
        resumeVM();
        dumpRes = waitDumping(pid);
        KLog.i(TAG, "hprof pid:" + pid + " dumped: " + path);
      }

    } catch (IOException e) {
      e.printStackTrace();
      KLog.e(TAG, "dump failed caused by IOException!");
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
  private native void initForkDump(int sdk_version);

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
