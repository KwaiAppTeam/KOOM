package com.kwai.koom.javaoom.report;

import java.lang.ref.WeakReference;
import java.util.Map;

import org.jetbrains.annotations.NotNull;

import android.app.Activity;
import android.app.Application;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.text.TextUtils;

import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KUtils;
import com.kwai.koom.javaoom.common.RunningInfoFetcher;

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
public class DefaultRunningInfoFetcher implements RunningInfoFetcher {

  String appVersion;

  @Override
  public String appVersion() {
    if (!TextUtils.isEmpty(appVersion)) {
      return appVersion;
    }
    try {
      appVersion = KGlobalConfig.getApplication().getPackageManager()
          .getPackageInfo(KGlobalConfig.getApplication().getPackageName(), 0)
          .versionName;
    } catch (PackageManager.NameNotFoundException e) {
      e.printStackTrace();
    }
    return appVersion;
  }

  @Override
  public String currentPage() {
    return (currentActivityWeakRef == null || currentActivityWeakRef.get() == null) ? ""
        : currentActivityWeakRef.get().getLocalClassName();
  }

  @Override
  public Integer usageSeconds() {
    return KUtils.usageSeconds();
  }

  @Override
  public Map<String, String> ext() {
    return null;
  }

  private WeakReference<Activity> currentActivityWeakRef;

  private void updateCurrentActivityWeakRef(Activity activity) {
    if (currentActivityWeakRef == null) {
      currentActivityWeakRef = new WeakReference<>(activity);
    } else {
      currentActivityWeakRef = currentActivityWeakRef.get() == activity ?
          currentActivityWeakRef : new WeakReference<>(activity);
    }
  }

  public DefaultRunningInfoFetcher(Application application) {
    application.registerActivityLifecycleCallbacks(new Application.ActivityLifecycleCallbacks() {
      @Override
      public void onActivityCreated(@NotNull Activity activity, Bundle savedInstanceState) {
        updateCurrentActivityWeakRef(activity);
      }

      @Override
      public void onActivityStarted(@NotNull Activity activity) {
        updateCurrentActivityWeakRef(activity);
      }

      @Override
      public void onActivityResumed(@NotNull Activity activity) {
        updateCurrentActivityWeakRef(activity);
      }

      @Override
      public void onActivityPaused(@NotNull Activity activity) {
        updateCurrentActivityWeakRef(activity);
      }

      @Override
      public void onActivityStopped(@NotNull Activity activity) {
        updateCurrentActivityWeakRef(activity);
      }

      @Override
      public void onActivitySaveInstanceState(@NotNull Activity activity,
          @NotNull Bundle outState) {
        updateCurrentActivityWeakRef(activity);
      }

      @Override
      public void onActivityDestroyed(@NotNull Activity activity) {
        updateCurrentActivityWeakRef(activity);
      }
    });
  }
}
