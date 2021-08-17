package com.kwai.performance.overhead.thread.monitor;

import androidx.annotation.WorkerThread;

public class NativeHandler {

  public native void init();

  public native void start();

  public native void refresh();

  @WorkerThread
  public native void stop();

  @WorkerThread
  public native void logThreadStatus(String type);

  public native void startCollect(String mode);

  public native void endCollect();

  public native void setJavaStackDumpTimeGap(int timeGap, int loop);

  public native void setNativeStackDumpTimeGap(int timeGap, int loop);

  public native void setThreadLeakDelay(long delay);

  public native void disableJavaStack();

  public native void disableNativeStack();

  public native void enableNativeLog();

  public native void enableThreadAddCustomLog();

  public native void enableSigSegvProtection();

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
