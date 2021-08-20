package com.kwai.koom.demo.threadleak

object ThreadLeakTest {
  /**
   * A native method that is implemented by the 'native-lib' native library,
   * which is packaged with this application.
   */
  @JvmStatic
  external fun triggerLeak(delay:Long)
}