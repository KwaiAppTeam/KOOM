# ThreadLeakMonitor 介绍

用于监控应用的线程泄漏问题，它的核心原理
- hook pthread_create/pthread_exit 等线程方法，用于记录线程的生命周期和创建堆栈，名称等信息
- 当发现一个joinable的线程在没有detach或者join的情况下，执行了pthread_exit，则记录下泄露线程信息
- 当线程泄露时间到达配置设置的延迟期限的时候，上报线程泄露信息

# ThreadLeakMonitor 适用范围
- Android N 及以上（API level >= 24）
- 仅支持 arm64-v8a

# ThreadLeakMonitor 接入
## 依赖配置
- 项目根目录 build.gradle 中增加 mavenCentral
```groovy
repositories {
    mavenCentral()
}
```
- 项目 app/build.gradle 中增加依赖
```groovy
dependencies {
  implementation "com.kuaishou.koom:koom-thread-leak:${latest_version}"
  implementation "com.kuaishou.koom:xhook:${latest_version}"
}
```
## 使用
- 初始化 MonitorManager

由于 ThreadLeakMonitor 依赖 MonitorManager，确保 MonitorManager 已经初始化

- 初始化 Monitor
```kotlin
......
val config = ThreadMonitorConfig.Builder()
        .enableThreadLeakCheck(30 * 1000L, 60 * 1000L) // 设置轮询间隔为30s，线程泄露延迟期限为1min
        .setListener(listener)
        .build()
MonitorManager.addMonitorConfig(config)
......
```
- 启动 Monitor，开始周期性的检测泄漏
```kotlin
......
ThreadMonitor.startTrackAsync()
......
```
- 停止 Monitor，通常不用主动停止
```kotlin
ThreadMonitor.stop()
```
- 在`ThreadLeakListener`中回调接收泄漏信息
```kotlin
val listener = object : ThreadLeakListener {
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
- 为什么不支持 Android N 以下的设备？
    - 考虑到线程监控的性能损耗和线程问题的共性，只监控 Android N 以上的设备应该可解决大多数问题
- 为什么不支持 armeabi-v7a
    - 获取内存分配堆栈时，出于性能的考虑我们使用了 FP Unwind(frame pointer unwind)；而 armeabi-v7a 采用 ARM/Thumb 混合指令，而 FP Unwind 在 ARM/Thumb 指令混合的方法栈上不可靠
    - AArch32(Arm 32-bit Architecture) 的 ABI 也没有规定 FP 的行为