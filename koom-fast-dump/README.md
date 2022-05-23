中文版本请参看[这里](README.zh-CN.md)

# Fast Dump Introduction

Java memory memory image dump module：

- Image dump adopts the strategy of `current process virtual machine supend -> fork child
    process -> current process virtual machine resume ->c hild process dump memory image`, and the 
  time-consuming (median about 20s) of collecting images is transferred to child process, the 
  freezing time of the current process is reduced to less than 20ms (does not affect user operations).

- Adapt to LeakCanary custom dumper, which can replace LeakCanary's image acquisition module with fast dump.

# Fast Dump Compatibility

- Android L and above（API level >= 21）

- Support armeabi-v7a arm64-v8a x86 x86-64


# Fast Dump Setup

## Dependencies

- Add mavenCentral to the repositories of the project root directory build.gradle
```groovy
repositories {
    mavenCentral()
}
```

- Add dependency in project app/build.gradle
```groovy
dependencies {
    implementation "com.kuaishou.koom:koom-fast-dump:${latest_version}"
}
```

## Steps

- Take the integration of fast dump into LeakCanary as an example：

```kotlin
// Configuration initialization
DefaultInitTask.init(applicationContext as Application)
LeakCanary.config = LeakCanary.config.copy(
  heapDumper = HeapDumper {
    // The core code is this line. Note that this method will wait for the child process to 
    // return the dump result, do not call it on the UI thread!
    ForkJvmHeapDumper.getInstance().dump(it.absolutePath)
  })
```


- Key Log
```kotlin
16743 16766 I OOMMonitor_ForkJvmHeapDumper: dump xxx.hprof.
16743 16766 I OOMMonitor_ForkJvmHeapDumper: before suspend and fork.
// The pid changes from 16743 to 16807, and the child process starts to dump
16807 16807 I mple.leakcanar: hprof: heap dump "xxx.hprof" starting...
// The child process dump finished, it takes 6.4s
16807 16807 I mple.leakcanar: hprof: heap dump completed (24MB) in 6.411s objects 330914 objects with stack traces 0
16807 16807 I JNIBridge: process 16807 will exit!
// The main process is notified by the completion of the child process dump
16743 16766 I OOMMonitor_ForkJvmHeapDumper: dump true, notify from pid 16807
```
