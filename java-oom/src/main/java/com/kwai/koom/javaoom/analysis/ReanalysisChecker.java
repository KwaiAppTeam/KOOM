package com.kwai.koom.javaoom.analysis;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import android.text.TextUtils;
import android.util.Log;

import com.google.gson.Gson;
import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.common.KUtils;
import com.kwai.koom.javaoom.report.HeapReport;

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
 * See KHeapFile, each contains a pair of hprof and report which file prefix is same.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class ReanalysisChecker {

  private static final String TAG = "ReanalysisChecker";

  public KHeapFile detectReanalysisFile() {
    File reportDir = new File(KGlobalConfig.getReportDir());
    File[] reports = reportDir.listFiles();
    if (reports == null) {
      return null;
    }
    for (File report : reports) {
      HeapReport heapReport = loadFile(report);
      if (analysisNotDone(heapReport)) {
        if (!overReanalysisMaxTimes(heapReport)) {
          KLog.i(TAG, "find reanalyze report");
          return buildKHeapFile(report);
        } else {
          KLog.e(TAG, "Reanalyze " + report.getName() + " too many times");
          //Reanalyze too many times, and the hporf is abnormal, so delete them.
          File hprof = findHprof(getReportFilePrefix(report));
          if (hprof != null) {
            hprof.delete();
          }
          report.delete();
        }
      }
    }
    return null;
  }

  private String getReportFilePrefix(File report) {
    int JSON_LEN = ".json".length();
    String reportName = report.getName();
    return reportName.substring(0, reportName.length() - JSON_LEN);
  }

  private File findHprof(String prefix) {
    int HPROF_LEN = ".hprof".length();
    File hprofDir = new File(KGlobalConfig.getHprofDir());
    File[] hprofs = hprofDir.listFiles();
    if (hprofs == null) {
      return null;
    }
    for (File hprof : hprofs) {
      String hprofName = hprof.getName();
      String hprofPrefix = hprofName.substring(0, hprofName.length() - HPROF_LEN);
      if (TextUtils.equals(prefix, hprofPrefix)) {
        return hprof;
      }
    }
    return null;
  }

  private KHeapFile buildKHeapFile(File report) {
    String reportPrefix = getReportFilePrefix(report);
    File hprof = findHprof(reportPrefix);
    if (hprof != null) {
      return KHeapFile.buildInstance(hprof, report);
    } else {
      KLog.e(TAG, "Reanalyze hprof file not found!");
      report.delete();
    }
    return null;
  }

  private boolean overReanalysisMaxTimes(HeapReport heapReport) {
    return heapReport.reAnalysisTimes != null && heapReport.reAnalysisTimes
        >= KConstants.ReAnalysis.MAX_TIMES;
  }

  private boolean analysisNotDone(HeapReport heapReport) {
    return heapReport.analysisDone == null || !heapReport.analysisDone;
  }

  private HeapReport loadFile(File file) {
    Gson gson = new Gson();
    FileInputStream fin = null;
    try {
      fin = new FileInputStream(file);
      int len = fin.available();
      byte[] bytes = new byte[len];
      fin.read(bytes);
      String str = new String(bytes);
      if (KConstants.Debug.VERBOSE_LOG) {
        KLog.i(TAG, "loadFile " + file.getPath() + " str:" + str);
      }
      HeapReport heapReport = gson.fromJson(str, HeapReport.class);
      return heapReport == null ? new HeapReport() : heapReport;
    } catch (Exception e) {
      file.delete();
    } finally {
      KUtils.closeQuietly(fin);
    }
    return new HeapReport();
  }
}
