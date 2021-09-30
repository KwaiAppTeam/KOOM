中文版本请参看[这里](README.zh-CN.md)

# ThreadLeakMonitor Introduction

Used to monitor the application thread leakage problem, its core principle
-Hook pthread_create/pthread_exit and other thread methods, used to record the life cycle of the thread and create stack, name and other information
-When it is found that a joinable thread executes pthread_exit without detach or join, record the leaked thread information
-When the thread leak time reaches the delay period set by the configuration, report thread leak information

# ThreadLeakMonitor scope of application
-Android N and above (API level >= 24)
-Only support arm64-v8a

# ThreadLeakMonitor access
## Dependent configuration
- Add mavenCentral to the repositories of the project root directory build.gradle
```groovy
repositories {
    mavenCentral()
}
```

- Add dependency in project app/build.gradle
```groovy
dependencies {
    implementation "com.kuaishou.koom:koom-thread-leak:${latest_version}"
    implementation "com.kuaishou.koom:xhook:${latest_version}"
}
```
## use
-Initialize MonitorManager

Since ThreadLeakMonitor depends on MonitorManager, make sure that MonitorManager has been initialized

-Initialize Monitor
```kotlin
......
val config = ThreadMonitorConfig.Builder()
        .enableThreadLeakCheck(30 * 1000L, 60 * 1000L) // Set the polling interval to 30s, and the thread leak delay period to 1min
        .setListener(listener)
        .build()
MonitorManager.addMonitorConfig(config)
......
```
-Start Monitor and start periodic leak detection
```kotlin
......
ThreadMonitor.startTrackAsync()
......
```
-Stop Monitor, usually you don’t need to stop actively
```kotlin
ThreadMonitor.stop()
```
-Call back in `ThreadLeakListener` to receive leaked information
```kotlin
val listener = object: ThreadLeakListener {
  override fun onReport(leaks: MutableList<ThreadLeakRecord>) {
    leaks.forEach {
      MonitorLog.i(ThreadLeakTestActivity.LOG_TAG, it.toString())
    }
  }
  override fun onError(msg: String) {
    MonitorLog.e(ThreadLeakTestActivity.LOG_TAG, msg)
  }
}
```
# FAQ
-Why are devices below Android N not supported?
-Taking into account the performance loss of thread monitoring and the commonality of thread problems, monitoring only devices above Android N should solve most problems
-Why not support armeabi-v7a
-When obtaining the memory allocation stack, we used FP Unwind (frame pointer unwind) for performance considerations; while armeabi-v7a uses ARM/Thumb mixed instructions, while FP Unwind is unreliable on the ARM/Thumb instruction mixed method stack
-The ABI of AArch32 (Arm 32-bit Architecture) also does not specify the behavior of FP