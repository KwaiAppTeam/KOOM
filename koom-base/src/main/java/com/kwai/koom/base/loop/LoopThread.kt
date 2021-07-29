package com.kwai.koom.base.loop

import android.os.Handler
import android.os.HandlerThread
import android.os.Process.THREAD_PRIORITY_BACKGROUND

internal object LoopThread : HandlerThread("LoopThread", THREAD_PRIORITY_BACKGROUND) {
  init {
    start()
  }

  internal val LOOP_HANDLER = Handler(LoopThread.looper)
}