package com.kwai.koom.javaoom.analysis;

import com.kwai.koom.javaoom.common.KUtils;

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
public abstract class LeakDetector {

  boolean VERBOSE_LOG = true;

  /**
   * LeakDetector try's to find the detect class's leaked instance and first we need to judge
   * whether the object's class is sub class of LeakDetector's detect class.
   * <p>
   * See generation(), if the instance's class is sub class of detect class, then it's
   * 'Generation' position class id  is same with the detect class.
   * <p>
   * Using generation to judge subclass instance is the relatively more efficient way.
   *
   * @param classId instance class id
   * @return whether the instance class id is sub class of this leak detector's detect class.
   */
  boolean isSubClass(long classId) {
    return ClassHierarchyFetcher.getIdOfGeneration(classId, generation()) == classId();
  }

  /**
   * Judge whether the instance is leak or not.
   *
   * @param instance judged instance
   * @return instance is leak or not
   */
  abstract boolean isLeak(HeapObject.HeapInstance instance);

  /**
   * ClassCounter contains all instances count and leaked instances count.
   *
   * @return instance count
   */
  public abstract ClassCounter instanceCount();

  /**
   * Generation is a fixed number, here we define the detect class's position
   * in the class hierarchy as 'Generation'.
   * <p>
   * For example :
   * <p>
   * TestClass1 extends Object {
   * <p>
   * }
   * <p>
   * TestClass2 extends TestClass1 {
   * <p>
   * }
   * <p>
   * Here TestClass1's generation is 1, and TestClass2's generation is 2.
   *
   * @return Generation
   */
  int storedGeneration = 0;

  public int generation() {
    if (storedGeneration != 0) {
      return storedGeneration;
    }
    return storedGeneration = KUtils.computeGenerations(clazz());
  }

  /**
   * Detected class' id.
   *
   * @return class id
   */
  public abstract long classId();

  /**
   * Detected class's name.
   *
   * @return class name
   */
  public abstract String className();

  /**
   * Detected class.
   *
   * @return class
   */
  public abstract Class<?> clazz();

  /**
   * Detected current leaked instance's reason.
   *
   * @return leak reason
   */
  public abstract String leakReason();
}
