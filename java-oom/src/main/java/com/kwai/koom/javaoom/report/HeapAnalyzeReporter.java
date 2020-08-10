package com.kwai.koom.javaoom.report;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.os.Build;
import android.os.Debug;
import android.util.Log;
import android.util.Pair;

import com.google.gson.Gson;
import com.kwai.koom.javaoom.analysis.LeakDetector;
import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.common.KUtils;
import com.kwai.koom.javaoom.monitor.TriggerReason;

import kshark.ApplicationLeak;
import kshark.Leak;
import kshark.LeakTrace;
import kshark.LeakTraceObject;
import kshark.LeakTraceReference;
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
 * <p>
 * HeapAnalyzeReporter organize info in the heap report file.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class HeapAnalyzeReporter {

  private static final String TAG = "HeapAnalyzeReporter";

  private static HeapAnalyzeReporter instance;
  private File reportFile;
  private HeapReport heapReport;
  private Gson gson;

  public HeapAnalyzeReporter() {
    gson = new Gson();
    reportFile = KHeapFile.getKHeapFile().report.file();
    heapReport = loadFile();
    if (heapReport == null) {
      heapReport = new HeapReport();
    }
  }

  private static HeapAnalyzeReporter getInstance() {
    return instance == null ? instance =
        new HeapAnalyzeReporter() : instance;
  }

  private void flushFile() {
    FileOutputStream fos = null;
    try {
      String str = gson.toJson(heapReport);
      fos = new FileOutputStream(reportFile);
      KLog.i(TAG, "flushFile " + reportFile.getPath() + " str:" + str);
      fos.write(str.getBytes());
    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      KUtils.closeQuietly(fos);
    }
  }

  private HeapReport loadFile() {
    FileInputStream fin = null;
    try {
      fin = new FileInputStream(reportFile);
      int len = fin.available();
      byte[] bytes = new byte[len];
      fin.read(bytes);
      String str = new String(bytes);
      if (KConstants.Debug.VERBOSE_LOG) {
        KLog.i(TAG, "loadFile " + reportFile.getPath() + " str:" + str);
      }
      return gson.fromJson(str, HeapReport.class);
    } catch (IOException e) {
      //e.printStackTrace();
    } finally {
      KUtils.closeQuietly(fin);
    }
    return new HeapReport();
  }

  private HeapReport.RunningInfo getRunningInfo() {
    return heapReport.runningInfo == null ? (heapReport.runningInfo =
        new HeapReport.RunningInfo()) : heapReport.runningInfo;
  }

  private void addRunningInfoInternal() {
    KLog.i(TAG, "addRunningInfoInternal");
    HeapReport.RunningInfo runningInfo = getRunningInfo();
    runningInfo.buildModel = Build.MODEL;
    runningInfo.manufacture = Build.MANUFACTURER;
    runningInfo.sdkInt = Build.VERSION.SDK_INT;

    runningInfo.usageSeconds = KGlobalConfig.getRunningInfoFetcher().usageSeconds();
    runningInfo.currentPage = KGlobalConfig.getRunningInfoFetcher().currentPage();
    runningInfo.appVersion = KGlobalConfig.getRunningInfoFetcher().appVersion();
    runningInfo.nowTime = KUtils.getTimeStamp();

    runningInfo.jvmMax = (int) (Runtime.getRuntime().maxMemory() / KConstants.Bytes.MB);
    runningInfo.jvmUsed =
        (int) ((Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory()) /
            KConstants.Bytes.MB);

    runningInfo.pss = (int) (Debug.getPss() / KConstants.Bytes.KB);
    KUtils.ProcessStatus processStatus = KUtils.getProcessMemoryUsage();
    runningInfo.vss = (int) (processStatus.vssKbSize / KConstants.Bytes.KB);
    runningInfo.rss = (int) (processStatus.rssKbSize / KConstants.Bytes.KB);
    runningInfo.threadCount = processStatus.threadsCount;

    runningInfo.koomVersion = KConstants.KOOMVersion.CODE;

    heapReport.runningInfo = runningInfo;

    flushFile();
  }

  public static void addDeviceRunningInfo() {
    getInstance().addRunningInfoInternal();
  }

  private void addDumpReasonInternal(TriggerReason.DumpReason reason) {
    HeapReport.RunningInfo runningInfo = getRunningInfo();
    runningInfo.dumpReason = reason.name();
    flushFile();
  }

  public static void addDumpReason(TriggerReason.DumpReason reason) {
    getInstance().addDumpReasonInternal(reason);
  }

  private void addAnalysisReasonInternal(TriggerReason.AnalysisReason reason) {
    HeapReport.RunningInfo runningInfo = getRunningInfo();
    runningInfo.analysisReason = reason.name();
    flushFile();
  }

  public static void addAnalysisReason(TriggerReason.AnalysisReason reason) {
    getInstance().addAnalysisReasonInternal(reason);
  }

  private void addClassInfoInternal(List<LeakDetector> leakDetectors) {
    KLog.i(TAG, "addClassInfoInternal");
    heapReport.classInfos = new ArrayList<>();
    for (LeakDetector leakDetector : leakDetectors) {
      HeapReport.ClassInfo classInfo = new HeapReport.ClassInfo();
      classInfo.className = leakDetector.className();
      classInfo.instanceCount = leakDetector.instanceCount().instancesCount;
      classInfo.leakInstanceCount = leakDetector.instanceCount().leakInstancesCount;
      heapReport.classInfos.add(classInfo);
      KLog.i(TAG, "class:" + classInfo.className + " all instances:" + classInfo.instanceCount
          + ", leaked instances:" + classInfo.leakInstanceCount);
    }
    flushFile();
  }

  public static void addClassInfo(List<LeakDetector> leakDetectors) {
    getInstance().addClassInfoInternal(leakDetectors);
  }

  private <T extends Leak> void addLeaks(List<T> leaks, Map<Long, String> reasonTable) {
    if (leaks == null || leaks.size() == 0) {
      return;
    }

    KLog.i(TAG, "add " + (leaks.get(0) instanceof ApplicationLeak ?
        "ApplicationLeak " : "LibraryLeak ") + leaks.size() + " leaks");

    for (Leak leak : leaks) {
      HeapReport.GCPath gcPath = new HeapReport.GCPath();
      heapReport.gcPaths.add(gcPath);
      gcPath.signature = leak.getSignature();
      gcPath.instanceCount = leak.getLeakTraces().size();

      //fetch the first trace which stands for this kind leak.
      LeakTrace trace = leak.getLeakTraces().get(0);
      String gcRoot = trace.getGcRootType().getDescription();
      gcPath.gcRoot = gcRoot;

      LeakTraceObject leakingObject = trace.getLeakingObject();
      String leakObjClazz = leakingObject.getClassName();
      String leakObjType = leakingObject.getTypeName();
      KLog.i(TAG, "GC Root:" + gcRoot
          + ", leakObjClazz:" + leakObjClazz
          + ", leakObjType:" + leakObjType
          + ", leaking reason:" + leakingObject.getLeakingStatusReason()
          + ", leaking id:" + (leakingObject.getObjectId() & 0xffffffffL));

      gcPath.leakReason = reasonTable.get(leakingObject.getObjectId())
          + (leak instanceof ApplicationLeak ? "" : " "
          + leakingObject.getLeakingStatusReason());
      gcPath.path = new ArrayList<>();

      HeapReport.GCPath.PathItem leakObjPathItem = new HeapReport.GCPath.PathItem();
      leakObjPathItem.reference = leakObjClazz;
      leakObjPathItem.referenceType = leakObjType;

      for (LeakTraceReference reference : trace.getReferencePath()) {
        String referenceName = reference.getReferenceName();
        String clazz = reference.getOriginObject().getClassName();
        String referenceDisplayName = reference.getReferenceDisplayName();
        String referenceGenericName = reference.getReferenceGenericName();
        String referenceType = reference.getReferenceType().toString();
        String declaredClassName = reference.getDeclaredClassName();
        KLog.i(TAG, "clazz:" + clazz + ", referenceName:" + referenceName
            + ", referenceDisplayName:" + referenceDisplayName
            + ", referenceGenericName:" + referenceGenericName
            + ", referenceType:" + referenceType
            + ", declaredClassName:" + declaredClassName);

        HeapReport.GCPath.PathItem leakPathItem = new HeapReport.GCPath.PathItem();
        leakPathItem.reference = referenceDisplayName.startsWith("[") ? clazz ://数组类型[]
            clazz + "." + referenceDisplayName;
        leakPathItem.referenceType = referenceType;
        leakPathItem.declaredClass = declaredClassName;
        gcPath.path.add(leakPathItem);
      }
      gcPath.path.add(leakObjPathItem);
    }
  }

  private void addGCPathInternal(Pair<List<ApplicationLeak>, List<LibraryLeak>> leaks,
      Map<Long, String> reasonTable) {
    if (heapReport.gcPaths == null) {
      heapReport.gcPaths = new ArrayList<>();
    }

    addLeaks(leaks.first, reasonTable);
    addLeaks(leaks.second, reasonTable);

    flushFile();
  }

  public static void addGCPath(Pair<List<ApplicationLeak>, List<LibraryLeak>> leaks,
      Map<Long, String> reasonTable) {
    getInstance().addGCPathInternal(leaks, reasonTable);
  }

  private void doneInternal() {
    heapReport.analysisDone = true;
    flushFile();
  }

  public static void done() {
    getInstance().doneInternal();
  }

  private void reAnalysisInternal() {
    KLog.i(TAG, "reAnalysisInternal");
    heapReport.reAnalysisTimes = heapReport.reAnalysisTimes
        == null ? 1 : heapReport.reAnalysisTimes + 1;
    flushFile();
  }

  public static void recordReanalysis() {
    getInstance().reAnalysisInternal();
  }
}
