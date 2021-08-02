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
package com.kwai.koom.base.loop

import android.os.Handler
import com.kwai.koom.base.Monitor
import java.util.concurrent.Callable

abstract class LoopMonitor<C> : Monitor<C>(), Callable<LoopMonitor.LoopState> {
  companion object {
    private const val DEFAULT_LOOP_INTERVAL = 1000L
  }

  @Volatile
  private var mIsLoopStopped = true

  private val mLoopRunnable = object : Runnable {
    override fun run() {
      if (call() == LoopState.Terminate) {
        return
      }

      if (mIsLoopStopped) {
        return
      }

      getLoopHandler().removeCallbacks(this)
      getLoopHandler().postDelayed(this, getLoopInterval())
    }
  }

  open fun startLoop(
      clearQueue: Boolean = true,
      postAtFront: Boolean = false,
      delayMillis: Long = 0L
  ) {
    if (clearQueue) getLoopHandler().removeCallbacks(mLoopRunnable)

    if (postAtFront) {
      getLoopHandler().postAtFrontOfQueue(mLoopRunnable)
    } else {
      getLoopHandler().postDelayed(mLoopRunnable, delayMillis)
    }

    mIsLoopStopped = false
  }

  open fun stopLoop() {
    mIsLoopStopped = true

    getLoopHandler().removeCallbacks(mLoopRunnable)
  }

  protected open fun getLoopInterval(): Long {
    return DEFAULT_LOOP_INTERVAL
  }

  protected open fun getLoopHandler(): Handler {
    return commonConfig.loopHandlerInvoker()
  }

  sealed class LoopState {
    object Continue : LoopState()

    object Terminate : LoopState()
  }
}