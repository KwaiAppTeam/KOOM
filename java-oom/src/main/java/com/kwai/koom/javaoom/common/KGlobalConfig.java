package com.kwai.koom.javaoom.common;

import java.io.File;

import android.app.Application;

import com.kwai.koom.javaoom.monitor.HeapThreshold;
import com.kwai.koom.javaoom.report.DefaultRunningInfoFetcher;

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
public class KGlobalConfig {

  private static KGlobalConfig globalConfig;

  private KGlobalConfig() {}

  private Application application;

  private static KGlobalConfig getGlobalConfig() {
    return globalConfig == null ? globalConfig = new KGlobalConfig() : globalConfig;
  }

  public void setApplicationInternal(Application application) {
    this.application = application;
    this.runningInfoFetcher = new DefaultRunningInfoFetcher(application);
  }

  public static void setApplication(Application application) {
    getGlobalConfig().setApplicationInternal(application);
  }

  public static Application getApplication() {
    return getGlobalConfig().application;
  }

  private KConfig kConfig;

  public static void setKConfig(KConfig kConfig) {
    getGlobalConfig().setKConfigInternal(kConfig);
  }

  public void setKConfigInternal(KConfig kConfig) {
    this.kConfig = kConfig;
  }

  public static KConfig getKConfig() {
    return getGlobalConfig().kConfig;
  }

  public static HeapThreshold getHeapThreshold() {
    return getGlobalConfig().kConfig.getHeapThreshold();
  }

  static final String KOOM_DIR = "koom";
  static final String HPROF_DIR = "hprof";
  static final String REPORT_DIR = "report";

  private static String rootDir;
  private static String reportDir;
  private static String hprofDir;

  public static void setRootDir(String rootDir) {
    getGlobalConfig().kConfig.setRootDir(rootDir);
  }

  public static String getRootDir() {
    if (rootDir != null) {
      return rootDir;
    }
    return rootDir = getGlobalConfig().kConfig.getRootDir();
  }

  public static String getReportDir() {
    if (reportDir != null) {
      return reportDir;
    }
    return reportDir = getRootDir() + File.separator + REPORT_DIR;
  }

  public static String getHprofDir() {
    if (hprofDir != null) {
      return hprofDir;
    }
    return hprofDir = getRootDir() + File.separator + HPROF_DIR;
  }

  private RunningInfoFetcher runningInfoFetcher;

  public static RunningInfoFetcher getRunningInfoFetcher() {
    return getGlobalConfig().runningInfoFetcher;
  }

  private KSoLoader soLoader;

  public static void setSoLoader(KSoLoader soLoader) {
    getGlobalConfig().soLoader = soLoader;
  }

  public static KSoLoader getSoLoader() {
    KSoLoader kSoLoader = getGlobalConfig().soLoader;
    return kSoLoader == null ? getGlobalConfig().soLoader = new DefaultKSoLoader() : kSoLoader;
  }

}
