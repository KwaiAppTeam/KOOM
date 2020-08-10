package com.kwai.koom.javaoom.analysis;

import static com.kwai.koom.javaoom.common.KConstants.ArrayThreshold.DEFAULT_BIG_OBJECT_ARRAY;
import static com.kwai.koom.javaoom.common.KConstants.ArrayThreshold.DEFAULT_BIG_PRIMITIVE_ARRAY;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import android.util.Log;
import android.util.Pair;

import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.report.HeapAnalyzeReporter;

import kotlin.jvm.internal.Reflection;
import kotlin.reflect.KClass;
import kotlin.sequences.Sequence;
import kshark.AndroidReferenceMatchers;
import kshark.ApplicationLeak;
import kshark.GcRoot;
import kshark.HeapAnalyzer;
import kshark.HeapGraph;
import kshark.HeapObject;
import kshark.Hprof;
import kshark.HprofHeapGraph;
import kshark.LibraryLeak;

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
class SuspicionLeaksFinder {

  private static final String TAG = "LeaksFinder";

  private Set<Long> leakingObjects;
  private KHeapFile.Hprof hprofFile;
  private HeapGraph heapGraph;

  public SuspicionLeaksFinder(KHeapFile.Hprof hprof) {
    leakingObjects = new HashSet<>();
    leakDetectors = new ArrayList<>();
    computeGenerations = new HashSet<>();
    this.hprofFile = hprof;
  }

  private List<LeakDetector> leakDetectors;
  private Set<Integer> computeGenerations;
  public Map<Long, String> leakReasonTable;

  private void initLeakDetectors() {
    addDetector(new ActivityLeakDetector(heapGraph));
    addDetector(new FragmentLeakDetector(heapGraph));
    addDetector(new BitmapLeakDetector(heapGraph));
    addDetector(new NativeAllocationRegistryLeakDetector(heapGraph));
    addDetector(new WindowLeakDetector(heapGraph));
    ClassHierarchyFetcher.initComputeGenerations(computeGenerations);
    leakReasonTable = new HashMap<>();
  }

  private void addDetector(LeakDetector leakDetector) {
    leakDetectors.add(leakDetector);
    computeGenerations.add(leakDetector.generation());
  }

  public Pair<List<ApplicationLeak>, List<LibraryLeak>> find() {
    boolean indexed = buildIndex();
    if (!indexed) {
      return null;
    }

    initLeakDetectors();

    findLeaks();

    return findPath();
  }

  private static final int SAME_CLASS_LEAK_OBJECT_GC_PATH_THRESHOLD = 45;//同名类实例寻找gc path，实例个数阈值

  public void findLeaks() {
    KLog.i(TAG, "start find leaks");
    //遍历镜像的所有instance
    Sequence<HeapObject.HeapInstance> instances = heapGraph.getInstances();
    Iterator<HeapObject.HeapInstance> instanceIterator = instances.iterator();

    while (instanceIterator.hasNext()) {
      HeapObject.HeapInstance instance = instanceIterator.next();
      if (instance.isPrimitiveWrapper()) {
        continue;
      }

      ClassHierarchyFetcher.process(instance.getInstanceClassId(),
          instance.getInstanceClass().getClassHierarchy());

      for (LeakDetector leakDetector : leakDetectors) {
        if (leakDetector.isSubClass(instance.getInstanceClassId())
            && leakDetector.isLeak(instance)) {
          ClassCounter classCounter = leakDetector.instanceCount();
          if (classCounter.leakInstancesCount <=
              SAME_CLASS_LEAK_OBJECT_GC_PATH_THRESHOLD) {
            leakingObjects.add(instance.getObjectId());
            leakReasonTable.put(instance.getObjectId(), leakDetector.leakReason());
          }
        }
      }
    }

    //关注class和对应instance数量，加入json
    HeapAnalyzeReporter.addClassInfo(leakDetectors);

    findPrimitiveArrayLeaks();
    findObjectArrayLeaks();
  }

  private void findPrimitiveArrayLeaks() {
    //查找基本类型数组
    Iterator<HeapObject.HeapPrimitiveArray> iterator = heapGraph.getPrimitiveArrays().iterator();
    while (iterator.hasNext()) {
      HeapObject.HeapPrimitiveArray array = iterator.next();
      int arraySize = array.getArrayLength();
      if (arraySize >= DEFAULT_BIG_PRIMITIVE_ARRAY) {
        String arrayName = array.getArrayClassName();
        String typeName = array.getPrimitiveType().toString();
        KLog.e(TAG, "primitive arrayName:" + arrayName + " typeName:" + typeName
            + " objectId:" + (array.getObjectId() & 0xffffffffL)
            + " arraySize:" + arraySize);
        leakingObjects.add(array.getObjectId());
        leakReasonTable.put(array.getObjectId(), "primitive array size over threshold:"
            + arraySize + "," + arraySize / KConstants.Bytes.KB + "KB");
      }
    }
  }

  private void findObjectArrayLeaks() {
    //查找对象数组
    Iterator<HeapObject.HeapObjectArray> iterator = heapGraph.getObjectArrays().iterator();
    while (iterator.hasNext()) {
      HeapObject.HeapObjectArray array = iterator.next();
      int arraySize = array.getArrayLength();
      if (arraySize >= DEFAULT_BIG_OBJECT_ARRAY) {
        String arrayName = array.getArrayClassName();
        KLog.i(TAG, "object arrayName:" + arrayName
            + " objectId:" + array.getObjectId());
        leakingObjects.add(array.getObjectId());
        leakReasonTable.put(array.getObjectId(), "object array size " +
            "over threshold:" + arraySize);
      }
    }
  }

  @SuppressWarnings("unchecked")
  public Pair<List<ApplicationLeak>, List<LibraryLeak>> findPath() {
    KLog.i(TAG, "findPath object size:" + leakingObjects.size());
    HeapAnalyzer.FindLeakInput findLeakInput = new HeapAnalyzer.FindLeakInput(heapGraph,
        AndroidReferenceMatchers.Companion.getAppDefaults(),
        false, Collections.emptyList());
    kotlin.Pair<List<ApplicationLeak>, List<LibraryLeak>> pair =
        new HeapAnalyzer(step -> KLog.i(TAG, "step:" + step.name()))
            .findLeaks(findLeakInput, leakingObjects, true);
    return new Pair<>((List<ApplicationLeak>) pair.getFirst(),
        (List<LibraryLeak>) pair.getSecond());
  }

  private boolean buildIndex() {
    KLog.i(TAG, "build index file:" + hprofFile.path);

    if (hprofFile.file() == null || !hprofFile.file().exists()) {
      KLog.e(TAG, "hprof file is not exists : " + hprofFile.path + "!!");
      return false;
    }

    Hprof hprof = Hprof.Companion.open(hprofFile.file());
    KClass<GcRoot>[] gcRoots = new KClass[]{
        Reflection.getOrCreateKotlinClass(GcRoot.JniGlobal.class),
        //Reflection.getOrCreateKotlinClass(GcRoot.JavaFrame.class),
        Reflection.getOrCreateKotlinClass(GcRoot.JniLocal.class),
        //Reflection.getOrCreateKotlinClass(GcRoot.MonitorUsed.class),
        Reflection.getOrCreateKotlinClass(GcRoot.NativeStack.class),
        Reflection.getOrCreateKotlinClass(GcRoot.StickyClass.class),
        Reflection.getOrCreateKotlinClass(GcRoot.ThreadBlock.class),
        Reflection.getOrCreateKotlinClass(GcRoot.ThreadObject.class),
        Reflection.getOrCreateKotlinClass(GcRoot.JniMonitor.class)};
    heapGraph = HprofHeapGraph.Companion.indexHprof(hprof, null,
        kotlin.collections.SetsKt.setOf(gcRoots));

    return true;
  }

  public Map<Long, String> getLeakReasonTable() {
    return leakReasonTable;
  }
}
