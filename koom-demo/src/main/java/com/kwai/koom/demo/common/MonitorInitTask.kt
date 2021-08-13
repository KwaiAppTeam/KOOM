package com.kwai.koom.demo.common

import android.app.Application
import android.os.SystemClock
import android.util.Log
import com.kwai.koom.demo.nativeleak.LeakMonitorInitTask

object MonitorInitTask : InitTask {
  private const val TAG = "MonitorInitTask"

  override fun init(application: Application) {
    val start = SystemClock.elapsedRealtime()

    CommonInitTask.init(application)

    Log.i(TAG, "init common config cost = ${SystemClock.elapsedRealtime() - start}")

    LeakMonitorInitTask.init(application)

    Log.i(TAG, "init custom config cost = ${SystemClock.elapsedRealtime() - start}")
  }
}