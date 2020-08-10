package com.kwai.koom.javaoom.analysis;

import android.util.Log;
import androidx.fragment.app.Fragment;

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
public class FragmentLeakDetector extends LeakDetector {

  private static final String NATIVE_FRAGMENT_CLASS_NAME = "android.app.Fragment";
// native android Fragment, deprecated as of API 28.
  private static final String SUPPORT_FRAGMENT_CLASS_NAME = "android.support.v4.app.Fragment";
// pre-androidx, support library version of the Fragment implementation.
  private static final String ANDROIDX_FRAGMENT_CLASS_NAME = "androidx.fragment.app.Fragment";
// androidx version of the Fragment implementation

  private static final String FRAGMENT_MANAGER_FIELD_NAME = "mFragmentManager";
  private static final String FRAGMENT_MCALLED_FIELD_NAME = "mCalled";

  private long fragmentClassId;
  private String fragmentClassName;
  private ClassCounter fragmentCounter;

  private static final String TAG = "FragmentLeakDetector";

  public FragmentLeakDetector(HeapGraph heapGraph) {
    HeapObject.HeapClass fragmentHeapClass =
        heapGraph.findClassByName(ANDROIDX_FRAGMENT_CLASS_NAME);
    fragmentClassName = ANDROIDX_FRAGMENT_CLASS_NAME;
    if (fragmentHeapClass == null) {
      fragmentHeapClass = heapGraph.findClassByName(NATIVE_FRAGMENT_CLASS_NAME);
      fragmentClassName = NATIVE_FRAGMENT_CLASS_NAME;
    }
    if (fragmentHeapClass == null) {
      fragmentHeapClass = heapGraph.findClassByName(SUPPORT_FRAGMENT_CLASS_NAME);
      fragmentClassName = SUPPORT_FRAGMENT_CLASS_NAME;
    }
    assert fragmentHeapClass != null;
    fragmentClassId = fragmentHeapClass.getObjectId();
    fragmentCounter = new ClassCounter();
  }

  @Override
  public long classId() {
    return fragmentClassId;
  }

  @Override
  public String className() {
    return fragmentClassName;
  }

  @Override
  public String leakReason() {
    return "Fragment Leak";
  }

  @Override
  public boolean isLeak(HeapObject.HeapInstance instance) {
    if (VERBOSE_LOG) {
      KLog.i(TAG, "run isLeak");
    }

    fragmentCounter.instancesCount++;
    boolean leak = false;
    HeapField fragmentManager = instance.get(fragmentClassName, FRAGMENT_MANAGER_FIELD_NAME);
    if (fragmentManager != null && fragmentManager.getValue().getAsObject() == null) {
      HeapField mCalledField = instance.get(fragmentClassName, FRAGMENT_MCALLED_FIELD_NAME);
      boolean abnormal = mCalledField == null || mCalledField.getValue().getAsBoolean() == null;
      if (abnormal) {
        KLog.e(TAG, "ABNORMAL mCalledField is null");
        return false;
      }
      leak = mCalledField.getValue().getAsBoolean();
      if (leak) {
        if (VERBOSE_LOG) {
          KLog.e(TAG, "fragment leak : " + instance.getInstanceClassName());
        }
        fragmentCounter.leakInstancesCount++;
      }
    }
    return leak;
  }

  @Override
  public ClassCounter instanceCount() {
    return fragmentCounter;
  }

  private static final int GENERATION = 1;

  @Override
  public int generation() {
    return GENERATION;
  }

  @Override
  public Class<?> clazz() {
    return Fragment.class;
  }
}
