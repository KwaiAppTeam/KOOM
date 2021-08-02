package com.kwai.koom.demo.nativeleak.init

import android.app.Application
import android.os.SystemClock
import android.util.Log

object PerformanceInitTask : InitTask {
  private const val TAG = "PerformanceInitTask"

  override fun init(application: Application) {
    val start = SystemClock.elapsedRealtime()

    CommonInitTask.init(application)

    Log.i(TAG, "init common config cost = ${SystemClock.elapsedRealtime() - start}")

    LeakMonitorInitTask.init(application)

    Log.i(TAG, "init custom config cost = ${SystemClock.elapsedRealtime() - start}")
  }
}