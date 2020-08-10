package com.kwai.koom.javaoom.analysis;

import android.graphics.Bitmap;
import android.util.Log;

import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KLog;

import kshark.HeapField;
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
public class BitmapLeakDetector extends LeakDetector {

  private static final String BITMAP_CLASS_NAME = "android.graphics.Bitmap";
  private static final String TAG = "BitmapLeakDetector";

  private BitmapLeakDetector() {}

  private long bitmapClassId;
  private ClassCounter bitmapCounter;

  public BitmapLeakDetector(HeapGraph heapGraph) {
    //bitmap
    HeapObject.HeapClass bitmapHeapClass = heapGraph.findClassByName(BITMAP_CLASS_NAME);
    assert bitmapHeapClass != null;
    bitmapClassId = bitmapHeapClass.getObjectId();
    bitmapCounter = new ClassCounter();
  }

  @Override
  public long classId() {
    return bitmapClassId;
  }

  @Override
  public String className() {
    return BITMAP_CLASS_NAME;
  }

  @Override
  public Class<?> clazz() {
    return Bitmap.class;
  }

  @Override
  public String leakReason() {
    return "Bitmap Size";
  }

  @Override
  public boolean isLeak(HeapObject.HeapInstance instance) {
    if (VERBOSE_LOG) {
      KLog.i(TAG, "run isLeak");
    }

    bitmapCounter.instancesCount++;
    HeapField fieldWidth = instance.get(BITMAP_CLASS_NAME, "mWidth");
    HeapField fieldHeight = instance.get(BITMAP_CLASS_NAME, "mHeight");
    assert fieldHeight != null;
    assert fieldWidth != null;
    boolean abnormal = fieldHeight.getValue().getAsInt() == null
        || fieldWidth.getValue().getAsInt() == null;
    if (abnormal) {
      KLog.e(TAG, "ABNORMAL fieldWidth or fieldHeight is null");
      return false;
    }
    int width = fieldWidth.getValue().getAsInt();
    int height = fieldHeight.getValue().getAsInt();
    boolean suspicionLeak = width * height >= KConstants.BitmapThreshold.DEFAULT_BIG_BITMAP;
    if (suspicionLeak) {
      KLog.e(TAG, "bitmap leak : " + instance.getInstanceClassName() + " " +
          "width:" + width + " height:" + height);
      bitmapCounter.leakInstancesCount++;
    }
    return suspicionLeak;
  }

  @Override
  public ClassCounter instanceCount() {
    return bitmapCounter;
  }
}
