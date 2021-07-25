package com.kwai.android.base

class NativeLib {

    /**
     * A native method that is implemented by the 'base' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'base' library on application startup.
        init {
            System.loadLibrary("base")
        }
    }
}