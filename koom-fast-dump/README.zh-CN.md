# Fast Dump 介绍

Java 内存镜像采集模块：

- 镜像采集采用`当前进程虚拟机supend -> fork子进程 -> 当前进程虚拟机resume -> 子进程dump内存镜像`的策略，将采集镜像的耗时(中位数约20s)转移到
  子进程，当前进程冻结的时间缩减至20ms以内(不影响用户操作)。

- 适配LeakCanary custom dumper，可将LeakCanary的镜像采集模块替换为fast dump。

# Fast Dump 适用范围

- Android L 及以上（API level >= 21）

- 支持 armeabi-v7a arm64-v8a x86 x86-64

# Fast Dump 接入

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
  implementation "com.kuaishou.koom:koom-fast-dump:${latest_version}"
}
```

## 使用

- 以接入LeakCanary为例：

```kotlin
// 初始化配置
DefaultInitTask.init(applicationContext as Application)
LeakCanary.config = LeakCanary.config.copy(
  heapDumper = HeapDumper {
    // 核心代码就这一行，注意此方法会等待子进程返回采集结果，不要在UI线程调用！
    ForkJvmHeapDumper.getInstance().dump(it.absolutePath)
  })
```
## 关键日志

> 16743 16766 I OOMMonitor_ForkJvmHeapDumper: dump xxx.hprof. <br>
> 16743 16766 I OOMMonitor_ForkJvmHeapDumper: before suspend and fork. <br>
> // pid从16743变为16807，子进程开始采集 <br>
> 16807 16807 I mple.leakcanar: hprof: heap dump "xxx.hprof" starting... <br>
> // 子进程采集完毕，耗时6.4s <br>
> 16807 16807 I mple.leakcanar: hprof: heap dump completed (24MB) in 6.411s objects 330914 objects with stack traces 0 <br>
> 16807 16807 I JNIBridge: process 16807 will exit! <br>
> // 主进程监听到子进程采集完毕 <br>
> 16743 16766 I OOMMonitor_ForkJvmHeapDumper: dump true, notify from pid 16807 <br>