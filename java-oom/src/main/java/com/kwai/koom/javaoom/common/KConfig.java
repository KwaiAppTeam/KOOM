package com.kwai.koom.javaoom.common;

import static com.kwai.koom.javaoom.common.KGlobalConfig.KOOM_DIR;

import java.io.File;

import com.kwai.koom.javaoom.monitor.HeapThreshold;

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
public class KConfig {

  private HeapThreshold heapThreshold;

  private String rootDir;
  private String processName;

  public KConfig(HeapThreshold heapThreshold, String rootDir, String processName) {
    this.rootDir = rootDir;
    this.processName = processName;
    this.heapThreshold = heapThreshold;
  }

  public HeapThreshold getHeapThreshold() {
    return heapThreshold;
  }

  public String getRootDir() {
    return rootDir;
  }

  public void setRootDir(String rootDir) {
    this.rootDir = rootDir;
  }

  public String getProcessName() {
    return processName;
  }

  public static KConfig defaultConfig() {
    return new KConfigBuilder().build();
  }

  public static class KConfigBuilder {

    private float heapRatio;
    private int heapOverTimes;
    private int heapPollInterval;

    private String processName; //default main process name

    private String rootDir;

    public KConfigBuilder() {
      this.heapRatio = KConstants.HeapThreshold.getDefaultPercentRation();
      this.heapOverTimes = KConstants.HeapThreshold.OVER_TIMES;
      this.heapPollInterval = KConstants.HeapThreshold.POLL_INTERVAL;
      this.rootDir = KGlobalConfig.getApplication().getCacheDir()
          .getAbsolutePath() + File.separator + KOOM_DIR;
      File dir = new File(rootDir);
      if (!dir.exists()) dir.mkdirs();
      this.processName = KGlobalConfig.getApplication().getPackageName();
    }

    public KConfigBuilder heapRatio(float heapRatio) {
      this.heapRatio = heapRatio;
      return this;
    }

    public KConfigBuilder heapOverTimes(int heapOverTimes) {
      this.heapOverTimes = heapOverTimes;
      return this;
    }

    public KConfigBuilder rootDir(String dir) {
      this.rootDir = dir;
      return this;
    }

    public KConfigBuilder processName(String name) {
      this.processName = name;
      return this;
    }

    public KConfig build() {
      HeapThreshold heapThreshold = new HeapThreshold(heapRatio,
          heapOverTimes, heapPollInterval);
      return new KConfig(heapThreshold, this.rootDir, this.processName);
    }
  }

}
