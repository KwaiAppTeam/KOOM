package com.kwai.koom.javaoom.analysis;

import android.app.Activity;
import android.util.Log;

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
public class ActivityLeakDetector extends LeakDetector {

  private static final String TAG = "ActivityLeakDetector";

  private static final String ACTIVITY_CLASS_NAME = "android.app.Activity";
  private static final String FINISHED_FIELD_NAME = "mFinished";
  private static final String DESTROYED_FIELD_NAME = "mDestroyed";

  private long activityClassId;
  private ClassCounter activityCounter;

  private ActivityLeakDetector() {}

  public ActivityLeakDetector(HeapGraph heapGraph) {
    HeapObject.HeapClass activityClass = heapGraph.findClassByName(ACTIVITY_CLASS_NAME);
    assert activityClass != null;
    activityClassId = activityClass.getObjectId();
    activityCounter = new ClassCounter();
  }

  @Override
  public long classId() {
    return activityClassId;
  }

  @Override
  public Class<?> clazz() {
    return Activity.class;
  }

  @Override
  public String className() {
    return ACTIVITY_CLASS_NAME;
  }

  @Override
  public String leakReason() {
    return "Activity Leak";
  }

  @Override
  public boolean isLeak(HeapObject.HeapInstance instance) {
    if (VERBOSE_LOG) {
      KLog.i(TAG, "run isLeak");
    }

    activityCounter.instancesCount++;
    HeapField destroyField = instance.get(ACTIVITY_CLASS_NAME, DESTROYED_FIELD_NAME);
    HeapField finishedField = instance.get(ACTIVITY_CLASS_NAME, FINISHED_FIELD_NAME);
    assert destroyField != null;
    assert finishedField != null;
    boolean abnormal = destroyField.getValue().getAsBoolean() == null
        || finishedField.getValue().getAsBoolean() == null;
    if (abnormal) {
      KLog.e(TAG, "ABNORMAL destroyField or finishedField is null");
      return false;
    }

    boolean leak = destroyField.getValue().getAsBoolean()
        || finishedField.getValue().getAsBoolean();

    if (leak) {
      if (VERBOSE_LOG) {
        KLog.e(TAG, "activity leak : " + instance.getInstanceClassName());
      }
      activityCounter.leakInstancesCount++;
    }
    return leak;
  }

  @Override
  public ClassCounter instanceCount() {
    return activityCounter;
  }
}
