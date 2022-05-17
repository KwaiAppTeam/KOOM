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
 * @author KOOM Team
 *
 */
package com.kwai.koom.base

import android.app.Activity
import android.app.Application
import android.os.Bundle
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.ProcessLifecycleOwner
import com.kwai.koom.base.MonitorManager.getApplication
import java.lang.ref.WeakReference
import java.util.concurrent.CopyOnWriteArrayList

private var _currentActivity: WeakReference<Activity>? = null
val Application.currentActivity: Activity?
  get() = _currentActivity?.get()

private var _isForeground = false
val Application.isForeground
  get() = _isForeground

private val _lifecycleEventObservers = CopyOnWriteArrayList<LifecycleEventObserver>()
fun Application.registerProcessLifecycleObserver(observer: LifecycleEventObserver) =
    _lifecycleEventObservers.add(observer)
fun Application.unregisterProcessLifecycleObserver(observer: LifecycleEventObserver) =
    _lifecycleEventObservers.remove(observer)

fun sdkVersionMatch(): Boolean {
  return MonitorManager.commonConfig.sdkVersionMatch
}

internal fun registerApplicationExtension() {
  getApplication().registerActivityLifecycleCallbacks(object : Application.ActivityLifecycleCallbacks {
    private fun updateCurrentActivityWeakRef(activity: Activity) {
      _currentActivity = if (_currentActivity?.get() == activity) {
        _currentActivity
      } else {
        WeakReference(activity)
      }
    }

    override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) {
      updateCurrentActivityWeakRef(activity)
    }

    override fun onActivityStarted(activity: Activity) {}

    override fun onActivityResumed(activity: Activity) {
      updateCurrentActivityWeakRef(activity)
    }

    override fun onActivityPaused(activity: Activity) {}

    override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) {}

    override fun onActivityStopped(activity: Activity) {}

    override fun onActivityDestroyed(activity: Activity) {}
  })

  ProcessLifecycleOwner.get().lifecycle.addObserver(object : LifecycleEventObserver {
    override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
      when (event) {
        Lifecycle.Event.ON_START -> _isForeground = true
        Lifecycle.Event.ON_STOP -> _isForeground = false
        else -> Unit
      }

      for (lifecycleEventObserver in _lifecycleEventObservers) {
        lifecycleEventObserver.onStateChanged(source, event)
      }
    }
  })
}