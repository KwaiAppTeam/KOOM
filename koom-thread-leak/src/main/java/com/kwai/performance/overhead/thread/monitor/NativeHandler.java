package com.kwai.performance.overhead.thread.monitor;

import androidx.annotation.WorkerThread;

public class NativeHandler {

  public native void start();

  public native void refresh();

  @WorkerThread
  public native void stop();

  public native void setThreadLeakDelay(long delay);

  public native void disableJavaStack();

  public native void disableNativeStack();

  public native void enableNativeLog();

  private INativeCallback mNativeCallback = null;

  public void setNativeCallback(INativeCallback nativeCallback) {
    mNativeCallback = nativeCallback;
  }

  public static void nativeCallback(int type, String key, String value) {
    if (getInstance().mNativeCallback != null) {
      getInstance().mNativeCallback.nativeCallback(type, key, value);
    }
  }

  public static NativeHandler getInstance(){
    return SingleTonHoler.INSTANCE;
  }

  private static class SingleTonHoler{
    private static NativeHandler INSTANCE = new NativeHandler();
  }

}
