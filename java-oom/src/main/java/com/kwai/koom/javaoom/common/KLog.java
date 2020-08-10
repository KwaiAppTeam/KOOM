package com.kwai.koom.javaoom.common;

import android.util.Log;

public class KLog {

    public interface KLogger {
        void i(String TAG, String msg);

        void d(String TAG, String msg);

        void e(String TAG, String msg);
    }

    public static class DefaultLogger implements KLogger {
        @Override
        public void i(String TAG, String msg) {
            Log.i(TAG, msg);
        }

        @Override
        public void d(String TAG, String msg) {
            Log.d(TAG, msg);
        }

        @Override
        public void e(String TAG, String msg) {
            Log.e(TAG, msg);
        }
    }

    private static KLogger logger;

    public static void init(KLogger kLogger) {
        logger = kLogger;
    }

    public static void i(String TAG, String msg) {
        if (logger == null) init(new DefaultLogger());
        logger.i(TAG, msg);
    }

    public static void d(String TAG, String msg) {
        if (logger == null) init(new DefaultLogger());
        logger.d(TAG, msg);
    }

    public static void e(String TAG, String msg) {
        if (logger == null) init(new DefaultLogger());
        logger.e(TAG, msg);
    }
}
