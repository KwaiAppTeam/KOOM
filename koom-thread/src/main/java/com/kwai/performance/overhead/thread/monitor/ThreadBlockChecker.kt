package com.kwai.performance.overhead.thread.monitor

import com.kwai.performance.overhead.thread.monitor.utils.getStackTrace

internal class ThreadBlockChecker(private val mConfig:
ThreadMonitorConfig) {
  private val lastThreadBlockMap = HashMap<ThreadBlockInfo, Long>()

  companion object {
    private const val TAG = "ThreadMonitor"
    private const val NO_MORE_REPORT = -1L
  }

  fun handleBlockCheck(loopTimes: Long) {
    if (mConfig.threadBlockInternal <= 0 || loopTimes % mConfig.threadBlockInternal != 0L) {
      return
    }
    mConfig.logger.info(TAG, "handleBlockCheck start")
    getAllBlockThreadTrace().map {
      val info = ThreadBlockInfo(it.key.id, it.key.state, it.key.name, it.value)
      val count = (lastThreadBlockMap[info] ?: 0L)
      if (count == NO_MORE_REPORT) {
        info to NO_MORE_REPORT
      } else {
        info to count + 1
      }
    }.apply {
      lastThreadBlockMap.clear()
    }.filter {
      val noMoreReport = mConfig.threadBlockStep <= 0
      val shouldReport = it.second >= mConfig.threadBlockStart
          && (noMoreReport || (it.second - mConfig.threadBlockStart) % mConfig.threadBlockStep == 0L)
      lastThreadBlockMap[it.first] = if (shouldReport && noMoreReport) NO_MORE_REPORT else it.second
      shouldReport
    }.map {
      mConfig.logger.info(TAG, "thread blocked, ${it.first.name} : ${it.second}")
      ThreadBlockReport(it.first.id,
          it.first.state,
          it.first.name,
          getStackTrace(it.first.stack, 0),
          it.second)
    }.let {
      if (it.isNotEmpty()) {
        val type = "thread_block"
        mConfig.listener?.onReport(type, mConfig.gson.toJson(BlockThreadData(type, it)))
      }
    }
  }

  private fun getAllBlockThreadTrace(): MutableMap<Thread, Array<StackTraceElement>> {
    val result: MutableMap<Thread, Array<StackTraceElement>> = java.util.HashMap()
    try {
      val systemThreadGroup = ThreadGroup::class.java.getDeclaredField("systemThreadGroup").let {
        it.isAccessible = true
        it.get(null) as ThreadGroup
      }
      var count: Int = systemThreadGroup.activeCount()
      val threads = arrayOfNulls<Thread>(count + count / 2)
      count = systemThreadGroup.enumerate(threads)
      for (i in 0 until count) {
        val thread = threads[i]
        if (thread != null) {
          val state = thread.state
          if (state == Thread.State.BLOCKED || state == Thread
                  .State.WAITING || state == Thread.State.TIMED_WAITING) {
            result[thread] = thread.stackTrace
          }
        }
      }
    } catch (e: Throwable) {
      e.printStackTrace()
    }
    return result
  }
}