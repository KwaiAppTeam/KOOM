package com.kwai.koom.javaoom.analysis;

import java.util.List;

import android.util.Log;
import android.util.Pair;

import com.kwai.koom.javaoom.common.KHeapFile;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.report.HeapAnalyzeReporter;

import kshark.ApplicationLeak;
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
class KHeapAnalyzer {

  private static final String TAG = "HeapAnalyzer";

  private SuspicionLeaksFinder leaksFinder;

  public KHeapAnalyzer(KHeapFile heapFile) {
    leaksFinder = new SuspicionLeaksFinder(heapFile.hprof);
  }

  public boolean analyze() {
    KLog.i(TAG, "analyze");
    Pair<List<ApplicationLeak>, List<LibraryLeak>> leaks = leaksFinder.find();
    if (leaks == null) {
      return false;
    }

    //Add gc path to report file.
    HeapAnalyzeReporter.addGCPath(leaks, leaksFinder.leakReasonTable);

    //Add done flag to report file.
    HeapAnalyzeReporter.done();
    return true;
  }

}
