package com.kwai.koom.javaoom.analysis;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import android.util.Log;

import com.kwai.koom.javaoom.common.KLog;

import kotlin.sequences.Sequence;
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
class ClassHierarchyFetcher {

  private static final String TAG = "ClassHierarchyFetcher";

  private Map<Long, List<ClassGeneration>> classGenerations;

  public ClassHierarchyFetcher() {
    classGenerations = new HashMap<>();
  }

  private static ClassHierarchyFetcher instance;

  private static ClassHierarchyFetcher getInstance() {
    return instance == null ? instance = new ClassHierarchyFetcher() : instance;
  }

  private static Map<Long, List<ClassGeneration>> getClassGenerations() {
    return getInstance().classGenerations;
  }

  private Set<Integer> computeGenerations;

  /**
   * Only compute some certain generations as set for performance concern.
   *
   * @param computeGenerations Generations in class hierarchy need to be computed.
   */
  public static void initComputeGenerations(Set<Integer> computeGenerations) {
    KLog.i(TAG, "initComputeGenerations " + computeGenerationsToString(computeGenerations));
    getInstance().computeGenerations = computeGenerations;
  }

  private static String computeGenerationsToString(Set<Integer> computeGenerations) {
    String generations = "";
    for (Integer generation : computeGenerations) {
      generations += generation + ",";
    }
    return generations;
  }

  private static Set<Integer> getComputeGenerations() {
    return getInstance().computeGenerations;
  }

  public static void process(long classId,
      Sequence<HeapObject.HeapClass> classHierarchy) {
    List<ClassGeneration> generations = getClassGenerations().get(classId);
    if (generations != null) {
      return;
    }

    //Log.i(TAG, "process class hierarchy generations");

    generations = new ArrayList<>();

    //first iteration computes the longest hierarchy
    Iterator<HeapObject.HeapClass> iterator = classHierarchy.iterator();
    int maxGenerations = 0;
    while (iterator.hasNext()) {
      iterator.next();
      maxGenerations++;
    }

    //second iteration find needed generation and stored in class generations map
    int generationIndex = 0;
    Iterator<HeapObject.HeapClass> iteratorAgain = classHierarchy.iterator();
    Set<Integer> computeGenerations = getComputeGenerations();
    while (iteratorAgain.hasNext()) {
      HeapObject.HeapClass clazz = iteratorAgain.next();
      generationIndex++;
      for (Integer generation : computeGenerations) {
        //find needed generation by using 'maxGenerations - generation'
        if (generationIndex == maxGenerations - generation) {
          ClassGeneration classGeneration = new ClassGeneration();
          classGeneration.id = clazz.getObjectId();
          classGeneration.generation = generation;
          generations.add(classGeneration);
        }
      }
    }

    //if (generations.size() > 0) {
    //  KLog.i(TAG, "generations.size() > 0");
    //}

    getClassGenerations().put(classId, generations);
  }

  public static long getIdOfGeneration(long classId, int generation) {
    List<ClassGeneration> generations = getClassGenerations().get(classId);
    if (generations == null) {
      return 0;
    }
    for (ClassGeneration classGeneration : generations) {
      if (classGeneration.generation == generation) {
        return classGeneration.id;
      }
    }
    return 0;
  }

  /**
   * Generation means Object's subclass generation.
   */
  static class ClassGeneration {
    long id;
    int generation;
  }
}
