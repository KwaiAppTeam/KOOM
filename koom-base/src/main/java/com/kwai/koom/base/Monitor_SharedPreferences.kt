package com.kwai.koom.base

import android.content.SharedPreferences

val SharedPreferences.allKeys: Set<String>
  get() = MonitorManager.commonConfig.sharedPreferencesKeysInvoker(this)
