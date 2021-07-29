package com.kwai.koom.base

import android.os.Handler
import android.os.Looper
import kotlin.concurrent.thread

internal val mainHandler = Handler(Looper.getMainLooper())

fun async(delayMills: Long = 0L, block: () -> Unit) {
  if (delayMills != 0L) {
    mainHandler.postDelayed({
      MonitorManager.commonConfig.executorServiceInvoker?.invoke()?.submit(block)
          ?: thread { block() }
    }, delayMills)
  } else {
    MonitorManager.commonConfig.executorServiceInvoker?.invoke()?.submit(block)
        ?: thread { block() }
  }
}

fun postOnMainThread(delayMills: Long = 0L, block: () -> Unit) {
  mainHandler.postDelayed(block, delayMills)
}

fun runOnMainThread(block: () -> Unit) {
  if (Looper.myLooper() == Looper.getMainLooper()) {
    block()
  } else {
    mainHandler.post(block)
  }
}

fun postOnMainThread(delay: Long = 0L, runnable: Runnable) {
  mainHandler.postDelayed(runnable, delay)
}

fun removeCallbacks(runnable: Runnable) {
  mainHandler.removeCallbacks(runnable)
}
