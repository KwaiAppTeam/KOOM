package com.kwai.koom.demo.threadleak

import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.kwai.koom.base.MonitorLog
import com.kwai.koom.base.MonitorManager
import com.kwai.koom.base.loadSoQuietly
import com.kwai.koom.demo.R
import com.kwai.koom.demo.threadleak.ThreadLeakTest.triggerLeak
import com.kwai.performance.overhead.thread.monitor.ThreadLeakListener
import com.kwai.performance.overhead.thread.monitor.ThreadLeakRecord
import com.kwai.performance.overhead.thread.monitor.ThreadMonitor
import com.kwai.performance.overhead.thread.monitor.ThreadMonitorConfig


class ThreadLeakTestActivity : AppCompatActivity() {
  private val mErrorText by lazy {
    findViewById<TextView>(R.id.tv_error_msg).also {
      it.setTextColor(Color.RED)
    }
  }
  private val mLeakText by lazy { findViewById<TextView>(R.id.tv_leak_msg) }
  private val mTest1Btn by lazy { findViewById<View>(R.id.btn_leak_delay_1) }
  private val mTest10Btn by lazy { findViewById<View>(R.id.btn_leak_delay_10) }

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_thread_leak_test)
    initMonitor()
    ThreadMonitor.startTrackAsync()
    if (loadSoQuietly("native-leak-test")) {
      mTest1Btn.setOnClickListener {
        triggerLeak(100)
      }
      mTest10Btn.setOnClickListener {
        triggerLeak(10 * 1000)
      }
    } else {
      mErrorText.text = "load native-leak-test fail!"
    }
  }

  private fun initMonitor() {
    val listener = object : ThreadLeakListener {
      override fun onReport(leaks: MutableList<ThreadLeakRecord>) {
        leaks.forEach {
          MonitorLog.i(LOG_TAG, it.toString())
        }
        mLeakText.post {
          mLeakText.text = "leak threads: ${leaks.map { it.name }}"
        }
      }

      override fun onError(msg: String) {
        MonitorLog.e(LOG_TAG, msg)
        mErrorText.post {
          mErrorText.text = msg
        }
      }
    }
    if (!ThreadMonitor.isInitialized) {
      val config = ThreadMonitorConfig.Builder()
          .enableThreadLeakCheck(2 * 1000L, 5 * 1000L)
          .setListener(listener)
          .build()
      MonitorManager.addMonitorConfig(config)
    } else {
      ThreadMonitor.setListener(listener)
    }
  }

  override fun onDestroy() {
    ThreadMonitor.stop()
    super.onDestroy()
  }

  companion object {
    private const val LOG_TAG = "ThreadLeakTest"
    fun start(context: Context) {
      context.startActivity(Intent(context, ThreadLeakTestActivity::class.java))
    }
  }
}