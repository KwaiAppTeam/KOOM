package com.kwai.performance.overhead.thread.monitor;

interface INativeCallback {
  void nativeCallback(int type, String key, String value);
}