package com.kwai.koom.javaoom.common;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;

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
public class KVData {

  private static boolean inited;
  private static SharedPreferences spTriggers;
  private static SharedPreferences spLaunchTime;

  public static void init() {
    Application application = KGlobalConfig.getApplication();
    spTriggers = application.getSharedPreferences(KConstants.SP
        .TRIGGER_TIMES_NAME, Context.MODE_PRIVATE);
    spLaunchTime = application.getSharedPreferences(KConstants.SP
        .FIRST_LAUNCH_TIME_NAME, Context.MODE_PRIVATE);
    inited = true;
  }

  public static void addTriggerTime(String version) {
    if (!inited) {
      init();
    }
    int times = getTriggerTimes(version);
    spTriggers.edit().putInt(version, times + 1).apply();
  }

  public static int getTriggerTimes(String version) {
    if (!inited) {
      init();
    }
    return spTriggers.getInt(version, 0);
  }

  public static long firstLaunchTime(String version) {
    if (!inited) {
      init();
    }
    long time = spLaunchTime.getLong(version, 0);
    if (time == 0) {
      time = System.currentTimeMillis();
      spLaunchTime.edit().putLong(version, time).apply();
    }
    return time;
  }
}
