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
 * A jvm hprof dumper which use io hook to strip, primitive
 * array and Zygote/Image heap space will be stripped.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.hprof;

import java.io.IOException;

import android.os.Build;
import android.os.Debug;

import com.kwai.koom.base.MonitorLog;

public class StripHprofHeapDumper extends HeapDumper {

  private static final String TAG = "OOMMonitor_StripHprofHeapDumper";

  public StripHprofHeapDumper() {
    super();
    if (soLoaded) {
      initStripDump();
    }
  }

  @Override
  public boolean dump(String path) {
    MonitorLog.i(TAG, "dump " + path);
    if (!soLoaded) {
      MonitorLog.e(TAG, "dump failed caused by so not loaded!");
      return false;
    }

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
      MonitorLog.e(TAG, "dump failed caused by version net permitted!");
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
