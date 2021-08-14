package com.kwai.performance.overhead.thread.monitor;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.text.TextUtils;

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

  private static SharedPreferences sp;
  private static Application application;

  public static void init(Application application) {
    KVData.application = application;
    sp = application.getSharedPreferences("koom_sp", Context.MODE_PRIVATE);
  }

  public static void addTriggerTime(String version) {
    int times = getTriggerTimes(version);
    sp.edit().putInt(version + "_times", times + 1).apply();
  }

  public static int getTriggerTimes(String version) {
    return sp.getInt(version + "_times", 0);
  }

  public static long firstLaunchTime(String version) {
    long time = sp.getLong(version + "_first_time", 0);
    if (time == 0) {
      time = System.currentTimeMillis();
      sp.edit().putLong(version + "_first_time", time).apply();
    }
    return time;
  }

  public static String appVersion() {
    String appVersion = "";
    try {
      appVersion = application.getPackageManager()
              .getPackageInfo(application.getPackageName(), 0)
              .versionName;
    } catch (PackageManager.NameNotFoundException e) {
      e.printStackTrace();
    }
    return appVersion;
  }
}
