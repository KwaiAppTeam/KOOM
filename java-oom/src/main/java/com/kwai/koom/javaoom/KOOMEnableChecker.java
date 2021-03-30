package com.kwai.koom.javaoom;

import android.os.Build;
import android.text.TextUtils;
import android.util.Log;

import com.kwai.koom.javaoom.common.KConstants;
import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.common.KUtils;
import com.kwai.koom.javaoom.common.KVData;

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
 * EnableChecker decides whether the device can run koom this time.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class KOOMEnableChecker {

  private static KOOMEnableChecker runningChecker;

  public static KOOMEnableChecker get() {
    return runningChecker = (runningChecker == null
            ? new KOOMEnableChecker() : runningChecker);
  }

  /**
   * Versions below Android 5.0 and above android 10.0 are incompatible now.
   *
   * @return support
   */
  public boolean isVersionPermit() {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
        && Build.VERSION.SDK_INT < Build.VERSION_CODES.R;
  }

  /**
   * Each user can triggers most 3 times heap analysis as default.
   *
   * @return result
   */
  public boolean isMaxTimesOverflow() {
    String version = KGlobalConfig.getRunningInfoFetcher().appVersion();
    int times = KVData.getTriggerTimes(version);
    KLog.i("koom", "version:" + version + " triggered times:" + times);
    return times > KConstants.EnableCheck.TRIGGER_MAX_TIMES;
  }

  /**
   * Each version can trigger most in 15 days as default.
   * <p>
   * Because when 15 days after almost all kinds of report are uploaded already.
   *
   * @return expired
   */
  public boolean isDateExpired() {
    String version = KGlobalConfig.getRunningInfoFetcher().appVersion();
    long time = KVData.firstLaunchTime(version);
    KLog.i("koom", "version:" + version + " first launch time:" + time);
    return System.currentTimeMillis() - time >
        KConstants.EnableCheck.MAX_TIME_WINDOW_IN_DAYS * KConstants.Time.DAY_IN_MILLS;
  }

  /**
   * Koom runs when disk space is enough.
   *
   * @return enough
   */
  public boolean isSpaceEnough() {
    String dir = KGlobalConfig.getRootDir();
    float space = KUtils.getSpaceInGB(dir);
    if (KConstants.Debug.VERBOSE_LOG) {
      KLog.i("koom", "Disk space:" + space + "Gb");
    }
    return space > KConstants.Disk.ENOUGH_SPACE_IN_GB;
  }

  /**
   * KOOM java-oom focused on the JVM memory, so only main process are enabled default.
   * <p>
   * Other process's limit can be removed by set a custom KConfig.
   *
   * @return process permitted
   */
  public boolean isProcessPermitted() {
    String enabledProcess = KGlobalConfig.getKConfig().getProcessName();
    String runningProcess = KUtils.getProcessName();
    KLog.i("koom", "enabledProcess:" + enabledProcess + ", runningProcess:" + runningProcess);
    return TextUtils.equals(enabledProcess, runningProcess);
  }

  public enum Result {
    NORMAL,
    EXPIRED_DATE,
    EXPIRED_TIMES,
    SPACE_NOT_ENOUGH,
    PROCESS_NOT_ENABLED,
    OS_VERSION_NO_COMPATIBILITY,
  }

  private Result result;

  /**
   * Check if KOOM can start.
   *
   * @return check result
   */
  public static Result doCheck() {
    runningChecker = get();

    if (runningChecker.result != null) {
      return runningChecker.result;
    }

    if (!runningChecker.isVersionPermit()) {
      return runningChecker.result = Result.OS_VERSION_NO_COMPATIBILITY;
    }

    if (!runningChecker.isSpaceEnough()) {
      return runningChecker.result = Result.SPACE_NOT_ENOUGH;
    }

    if (runningChecker.isDateExpired()) {
      return runningChecker.result = Result.EXPIRED_DATE;
    }

    if (runningChecker.isMaxTimesOverflow()) {
      return runningChecker.result = Result.EXPIRED_TIMES;
    }

    if (!runningChecker.isProcessPermitted()) {
      return runningChecker.result = Result.PROCESS_NOT_ENABLED;
    }

    return Result.NORMAL;
  }
}
