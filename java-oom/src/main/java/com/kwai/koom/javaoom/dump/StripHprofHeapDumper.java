package com.kwai.koom.javaoom.dump;

import java.io.IOException;

import android.os.Debug;
import android.util.Log;

import com.kwai.koom.javaoom.KOOMEnableChecker;
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
 * A jvm hprof dumper which use io hook to strip, primitive
 * array and Zygote/Image heap space will be stripped.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class StripHprofHeapDumper implements HeapDumper {

  private static final String TAG = "StripHprofHeapDumper";

  private boolean soLoaded;

  public StripHprofHeapDumper() {
    soLoaded = KGlobalConfig.getSoLoader().loadLib("koom-java");
    if (soLoaded) {
      initStripDump();
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

    boolean dumpRes = false;
    try {
      hprofName(path);
      Debug.dumpHprofData(path);
      dumpRes = isStripSuccess();
    } catch (IOException e) {
      e.printStackTrace();
    }

    return dumpRes;
  }

  public native void initStripDump();

  public native void hprofName(String name);

  public native boolean isStripSuccess();
}
