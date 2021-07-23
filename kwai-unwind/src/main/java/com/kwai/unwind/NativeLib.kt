package com.kwai.unwind

class NativeLib {

    /**
     * A native method that is implemented by the 'unwind' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'unwind' library on application startup.
        init {
            System.loadLibrary("unwind")
        }
    }
}