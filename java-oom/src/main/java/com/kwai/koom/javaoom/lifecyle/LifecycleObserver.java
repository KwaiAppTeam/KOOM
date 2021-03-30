package com.kwai.koom.javaoom.lifecyle;

/**
 * A replacement of ProcessLifecycle by need of removing AndroidX.
 */
public interface LifecycleObserver {
    /**
     * Triggered as soon as app switched to foreground.
     */
    void onForeground();

    /**
     * Triggered as soon as app switched to background.
     */
    void onBackground();
}
