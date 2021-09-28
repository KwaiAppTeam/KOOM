/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by lbtrace on 2021.
 *
 */

package com.kwai.koom.nativeoom.leakmonitor.allocationtag

import android.app.Activity
import android.app.Application
import android.content.Context
import android.os.Bundle
import com.kwai.koom.base.MonitorManager.getApplication
import com.kwai.koom.base.currentActivity
import com.kwai.koom.nativeoom.leakmonitor.LeakRecord
import java.util.concurrent.ConcurrentHashMap

object AllocationTagLifecycleCallbacks : Application.ActivityLifecycleCallbacks {
  private val mAllocationTagInfoMap = ConcurrentHashMap<String, AllocationTagInfo>()

  private var mIsRegistered = false


  fun register() {
    if (mIsRegistered) {
      return
    }
    mIsRegistered = true

    getApplication().registerActivityLifecycleCallbacks(this)
    getApplication().currentActivity?.let { onActivityCreated(it, null) }
  }

  fun unregister() {
    mIsRegistered = false

    getApplication().unregisterActivityLifecycleCallbacks(this)
    mAllocationTagInfoMap.clear()
  }

  fun bindAllocationTag(allocationInfoMap: Map<String, LeakRecord>?) {
    if (allocationInfoMap.isNullOrEmpty()) {
      return
    }

    val allocationTagInfoList = mAllocationTagInfoMap.values.toList().reversed()

    for ((_, value) in allocationInfoMap) {
      for (allocationTagInfo in allocationTagInfoList) {
        value.tag = allocationTagInfo.searchTag(value.index) ?: continue

        break
      }
    }
  }

  override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) {
    if (mAllocationTagInfoMap.containsKey(activity.toString())) {
      return
    }

    if (isFirstActivityCreate()) {
      mAllocationTagInfoMap.clear()
    }

    mAllocationTagInfoMap[activity.toString()] = activity.toString().createAllocationTagInfo()
  }

  override fun onActivityStarted(activity: Activity) {}

  override fun onActivityResumed(activity: Activity) {}

  override fun onActivityPaused(activity: Activity) {}

  override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) {}

  override fun onActivityStopped(activity: Activity) {}

  override fun onActivityDestroyed(activity: Activity) {
    mAllocationTagInfoMap[activity.toString()]?.end()
  }

  private fun isFirstActivityCreate(): Boolean {
    for (allocationTagInfo in mAllocationTagInfoMap.values) {
      if (allocationTagInfo.endTime == -1L) {
        return false
      }
    }

    return true
  }
}