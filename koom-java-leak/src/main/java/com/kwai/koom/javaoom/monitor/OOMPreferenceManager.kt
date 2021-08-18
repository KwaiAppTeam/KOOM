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

package com.kwai.koom.javaoom.monitor

import android.content.SharedPreferences
import com.kwai.koom.base.MonitorBuildConfig
import com.kwai.koom.base.allKeys

internal object OOMPreferenceManager {
  private const val PREFERENCE_NAME = "koom_hprof_analysis"

  private val mPreferences by lazy { mSharedPreferencesInvoker(PREFERENCE_NAME) }

  private lateinit var mSharedPreferencesInvoker: (String) -> SharedPreferences
  private lateinit var mPrefix: String

  fun init(sharedPreferencesInvoker: (String) -> SharedPreferences) {
    mSharedPreferencesInvoker = sharedPreferencesInvoker
    mPrefix = "${MonitorBuildConfig.VERSION_NAME}_"
  }

  fun getAnalysisTimes(): Int {
    return mPreferences.getInt("${mPrefix}times", 0)
  }

  fun increaseAnalysisTimes() {
    mPreferences.edit()
        .also { clearUnusedPreference(mPreferences, it) }
        .putInt("${mPrefix}times", mPreferences.getInt("${mPrefix}times", 0) + 1)
        .apply()
  }

  fun getFirstLaunchTime(): Long {
    var time = mPreferences.getLong("${mPrefix}first_analysis_time", 0)
    if (time == 0L) {
      time = System.currentTimeMillis()
      setFirstLaunchTime(time)
    }
    return time
  }

  fun setFirstLaunchTime(time: Long) {
    if (mPreferences.contains("${mPrefix}first_analysis_time")) {
      return
    }

    return mPreferences.edit()
        .putLong("${mPrefix}first_analysis_time", time)
        .apply()
  }

  private fun clearUnusedPreference(
      preferences: SharedPreferences,
      editor: SharedPreferences.Editor
  ) {
    for (key in preferences.allKeys) {
      if (!key.startsWith(mPrefix)) {
        editor.remove(key)
      }
    }
  }
}