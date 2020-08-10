package com.kwai.koom.javaoom.report;

import java.util.List;

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
 * Heap report file json format.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class HeapReport {

  public RunningInfo runningInfo;

  //device and app running info
  public static class RunningInfo {
    //JVM info
    public Integer jvmMax;//jvm max memory in MB
    public Integer jvmUsed;//jvm used memory in MB

    //memory info
    public Integer vss;//vss memory in MB
    public Integer pss;//pss memory in MB
    public Integer rss;//rss memory in MB
    public Integer threadCount;

    //Device info
    public Integer sdkInt;
    public String manufacture;
    public String buildModel;

    //App info
    public String appVersion;
    public String currentPage;
    public Integer usageSeconds;
    public String nowTime;

    public String dumpReason;//heap dump trigger reason,
    public String analysisReason;//analysis trigger reason

    //KOOM Perf data
    public Integer koomVersion;
    public String filterInstanceTime;
    public String findGCPathTime;
  }

  public List<GCPath> gcPaths;//gc path of suspected objects

  /**
   * GC Path means path of object to GC Root, it can also be called as reference chain.
   */
  public static class GCPath {
    public Integer instanceCount;//instances number of same path to gc root
    public String leakReason;//reason of why instance is suspected
    public String gcRoot;
    public String signature;//signature are computed by the sha1 of reference chain
    public List<PathItem> path;

    //引用链Item
    public static class PathItem {
      String reference;//referenced instance's classname + filed name
      String referenceType;//such as INSTANCE_FIELD/ARRAY_ENTRY/STATIC_FIELD
      String declaredClass;//for cases when filed is inherited from ancestor's class.
    }
  }

  public List<ClassInfo> classInfos;//Class's instances count list

  /**
   * ClassInfo contains data which describes the instances number of the Class.
   */
  public static class ClassInfo {
    public String className;
    public Integer instanceCount;//All instances's count of this class.
    public Integer leakInstanceCount;//All leaked instances's count of this class.
  }

  public Boolean analysisDone;//flag to record whether hprof is analyzed already.
  public Integer reAnalysisTimes;//flag to record hprof reanalysis times.
}
