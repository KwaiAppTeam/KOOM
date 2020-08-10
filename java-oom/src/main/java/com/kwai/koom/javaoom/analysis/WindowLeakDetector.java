package com.kwai.koom.javaoom.analysis;

import android.util.Log;
import android.view.Window;

import com.kwai.koom.javaoom.common.KLog;

import kshark.HeapGraph;
import kshark.HeapObject;

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
public class WindowLeakDetector extends LeakDetector {

  private static final String TAG = "WindowLeakDetector";

  private static final String WINDOW_CLASS_NAME = "android.view.Window";
  private static final int GENERATION = 1;//Window->Object

  private WindowLeakDetector() {}

  private long windowClassId;
  private ClassCounter windowCounter;

  public WindowLeakDetector(HeapGraph heapGraph) {
    //window
    HeapObject.HeapClass windowClass = heapGraph.findClassByName(WINDOW_CLASS_NAME);
    assert windowClass != null;
    windowClassId = windowClass.getObjectId();
    windowCounter = new ClassCounter();
  }

  @Override
  public long classId() {
    return windowClassId;
  }

  @Override
  public String className() {
    return WINDOW_CLASS_NAME;
  }

  @Override
  public boolean isLeak(HeapObject.HeapInstance instance) {
    if (VERBOSE_LOG) {
      KLog.i(TAG, "run isLeak");
    }

    windowCounter.instancesCount++;
    return false;
  }

  @Override
  public String leakReason() {
    return "Window";
  }

  @Override
  public ClassCounter instanceCount() {
    return windowCounter;
  }

  @Override
  public int generation() {
    return GENERATION;
  }

  @Override
  public Class<?> clazz() {
    return Window.class;
  }
}
