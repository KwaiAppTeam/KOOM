package com.kwai.koom.javaoom.analysis;

import android.util.Log;

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
public class NativeAllocationRegistryLeakDetector extends LeakDetector {

  private static final String TAG = "NativeAllocation";

  private static final String NATIVE_ALLOCATION_CLASS_NAME =
      "libcore.util.NativeAllocationRegistry";
  private static final String NATIVE_ALLOCATION_CLEANER_THUNK_CLASS_NAME =
      "libcore.util.NativeAllocationRegistry$CleanerThunk";

  private static final int GENERATION = 1;//NativeAllocationRegistry->Object

  private NativeAllocationRegistryLeakDetector() {}

  private boolean supported;
  private long nativeAllocationClassId;
  private long nativeAllocationThunkClassId;
  private ClassCounter nativeAllocationCounter;

  public NativeAllocationRegistryLeakDetector(HeapGraph heapGraph) {
    if (VERBOSE_LOG) {
      KLog.i(TAG, "run isLeak");
    }

    //native allocation
    HeapObject.HeapClass nativeAllocationHeapClass =
        heapGraph.findClassByName(NATIVE_ALLOCATION_CLASS_NAME);
    HeapObject.HeapClass nativeAllocationThunkHeapClass =
        heapGraph.findClassByName(NATIVE_ALLOCATION_CLEANER_THUNK_CLASS_NAME);
    if (nativeAllocationHeapClass != null) {
      nativeAllocationClassId = nativeAllocationHeapClass.getObjectId();
    } else {
      supported = false;
    }
    if (nativeAllocationThunkHeapClass != null) {
      nativeAllocationThunkClassId = nativeAllocationThunkHeapClass.getObjectId();
    } else {
      supported = false;
    }
    nativeAllocationCounter = new ClassCounter();
    supported = true;
  }

  @Override
  public boolean isSubClass(long classId) {
    if (!supported) {
      return false;
    }
    long id = ClassHierarchyFetcher.getIdOfGeneration(classId, generation());
    return id == nativeAllocationClassId || id == nativeAllocationThunkClassId;
  }

  @Override
  public long classId() {
    return nativeAllocationClassId;
  }

  @Override
  public String className() {
    return NATIVE_ALLOCATION_CLASS_NAME;
  }

  @Override
  public String leakReason() {
    return "NativeAllocation";
  }

  @Override
  public boolean isLeak(HeapObject.HeapInstance instance) {
    if (!supported) {
      return false;
    }
    nativeAllocationCounter.instancesCount++;
    return false;
  }

  @Override
  public ClassCounter instanceCount() {
    return nativeAllocationCounter;
  }

  @Override
  public int generation() {
    return GENERATION;
  }

  @Override
  public Class<?> clazz() {
    //not exists in sdk
    return null;
  }
}
