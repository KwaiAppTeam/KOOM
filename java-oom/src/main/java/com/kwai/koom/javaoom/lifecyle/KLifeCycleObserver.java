package com.kwai.koom.javaoom.lifecyle;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import com.kwai.koom.javaoom.common.KGlobalConfig;
import com.kwai.koom.javaoom.common.KLog;

import org.jetbrains.annotations.NotNull;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

public class KLifeCycleObserver {

    private KLifeCycleObserver() {
        observers = new ArrayList<>();
        init();
    }

    private static KLifeCycleObserver lifeCycle;

    public static KLifeCycleObserver get() {
        return lifeCycle == null ? lifeCycle = new KLifeCycleObserver() : lifeCycle;
    }

    private List<LifecycleObserver> observers;

    public void addObserver(LifecycleObserver lifecycleObserver) {
        observers.add(lifecycleObserver);
    }

    public void removeObserver(LifecycleObserver lifecycleObserver) {
        observers.remove(lifecycleObserver);
    }

    private void init() {
        Application application = KGlobalConfig.getApplication();
        application.registerActivityLifecycleCallbacks(new Application.ActivityLifecycleCallbacks() {
            @Override
            public void onActivityCreated(@NotNull Activity activity, Bundle savedInstanceState) {
                update(Event.CREATE);
            }

            @Override
            public void onActivityStarted(@NotNull Activity activity) {
                update(Event.START);
            }

            @Override
            public void onActivityResumed(@NotNull Activity activity) {
                update(Event.RESUME);
            }

            @Override
            public void onActivityPaused(@NotNull Activity activity) {
                update(Event.PAUSE);
            }

            @Override
            public void onActivityStopped(@NotNull Activity activity) {
                update(Event.STOP);
            }

            @Override
            public void onActivitySaveInstanceState(@NotNull Activity activity, @NotNull Bundle outState) {
                //DO NOTHING
            }

            @Override
            public void onActivityDestroyed(@NotNull Activity activity) {
                update(Event.DESTROY);
            }
        });
    }

    private static final String TAG = "KLifeCycle";

    private int foregroundActivityCnt;
    private Status lastLifeCycleStatus = Status.BACKGROUND;

    private void update(Event event) {
        KLog.i(TAG, "update " + event);
        if (event == Event.RESUME) {
            foregroundActivityCnt++;
        } else if (event == Event.PAUSE) {
            foregroundActivityCnt--;
        }

        if (lastLifeCycleStatus == Status.BACKGROUND && isForeground()) {
            KLog.i(TAG, "foreground");
            callback(Status.FOREGROUND);
        }  else if (lastLifeCycleStatus == Status.FOREGROUND && isBackground()) {
            KLog.i(TAG, "background");
            callback(Status.BACKGROUND);
        }
    }

    private void callback(Status status) {
        lastLifeCycleStatus = status;
        for (LifecycleObserver observer : observers) {
            if (status == Status.FOREGROUND) {
                observer.onForeground();
            } else {
                observer.onBackground();;
            }
        }
    }

    private boolean isForeground() {
        return foregroundActivityCnt > 0;
    }

    private boolean isBackground() {
        return !isForeground();
    }
}
