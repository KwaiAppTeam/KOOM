中文版本请参看[这里](README.zh-CN.md)

# OOMMonitor Introduction

OOMMonitor is designed for solving Android Application's Java memory leak problems，and it's core concepts are:

- By polling Java heap size, running threads and fd size periodic, Hprof dump is triggered when the threshold is exceeded multiple consecutive times.

- Hprof dump use `Suspend ART VM->fork VM process->Resume ART VM->Dump Hprof` strategy, making the simple dump's 20 seconds process frozen reduced to within 20ms.

- Using shark to parse hprof file, but some optimization changes are added in shark. Leaking judge and leaking reference chain computing are executed on the device, and give a heap report finally.

# OOMMonitor Compatibility

- Android L and above（API level >= 21）

- Support armeabi-v7a arm64-v8a x86 x86-64


# OOMMonitor Quick up

## dependencies

- Add mavenCentral to the repositories of the project root directory build.gradle
```groovy
repositories {
    mavenCentral()
}
```

- Add dependency in project app/build.gradle
```groovy
dependencies {
    implementation "com.kuaishou.koom:koom-java-leak:${latest_version}"
}
```

## Steps

- MonitorManager Initialization

Make sure MonitorManager is initialized for OOMMonitor depends on it, such as Application, App version code is correctly initialized.

- OOMMonitor Initialization

```kotlin
val config = OOMMonitorConfig.Builder()
    .setThreadThreshold(50) //50 only for test! Please use default value!
    .setFdThreshold(300) // 300 only for test! Please use default value!
    .setHeapThreshold(0.9f) // 0.9f for test! Please use default value!
    .setVssSizeThreshold(1_000_000) // 1_000_000 for test! Please use default value!
    .setMaxOverThresholdCount(1) // 1 for test! Please use default value!
    .setAnalysisMaxTimesPerVersion(3) // Consider use default value！
    .setAnalysisPeriodPerVersion(15 * 24 * 60 * 60 * 1000) // Consider use default value！
    .setLoopInterval(5_000) // 5_000 for test! Please use default value!
    .setEnableHprofDumpAnalysis(true)
    .setHprofUploader(object: OOMHprofUploader {
      override fun upload(file: File, type: OOMHprofUploader.HprofType) {
        MonitorLog.e("OOMMonitor", "todo, upload hprof ${file.name} if necessary")
      }
    })
    .setReportUploader(object: OOMReportUploader {
      override fun upload(file: File, content: String) {
        MonitorLog.i("OOMMonitor", content)
        MonitorLog.e("OOMMonitor", "todo, upload report ${file.name} if necessary")
      }
    })
    .build()

MonitorManager.addMonitorConfig(config)
```

- Start OOMMonitor

```kotlin
OOMMonitor.startLoop(5_000L)
```

-  Stop OOMMonitor

```kotlin
OOMMonitor.stopLoop()
```

- Check Running Log
    - start loop，`OOMMonitor: startLoop()`
    - exceed the threshold，`OOMMonitor_ThreadOOMTracker: [meet condition] overThresholdCount:1, threadCount: 717`
    - dump triggered，`OOMMonitor: dumpAndAnalysis`
    - dump ended：`OOMMonitor: end hprof dump`
    - start hprof analyze：`OOMMonitor: start hprof analysis`
    - hprof analysis ended ：`OOMMonitor: heap analysis success, do upload`

# OOMMonitor FAQ

- How about OOMMonitor performance overhead？
    - First, dump hprof will no longer froze the process 
    - But, dump and analysis in the sub process may compute above 3 minutes which up to heap file size, and this will use one cpu thread and 200m memory maximum
    - So，a remote switch and a sample rate is suggested

- What object is considered leaked？
    - For Activity and Fragment, when object is destroyed but a reference chain to gc root still exist, the object is leaked
        - Activity destroy judgement rule，mFinished or mDestroyed is true
        - Fragment destroy judgement rule，mFragmentManager is null and mCalled is true, mCalled is true means the fragment lifecycle callback is done 

- How to strip hprof in the dump process to reduced the hprof file size mostly？

    - strip dump in the subprocess

        ```java
        ForkStripHeapDumper.getInstance().dump(path)
        ```

- How to refill the stripped hprof， make it available to AS Profiler and MAT？

    - fetch the hprof from the device

        ```shell
        adb shell "run-as com.kwai.koom.demo cat 'files/test.hprof'" > ~/temp/test.hprof
        ```

    - Use`tools/koom-fill-crop.jar` to refill the stripped hprof

        ```shell
        java -jar koom-fill-crop.jar test.hprof
        ```

- How to read the report file？

    - Please read the comments in the `HeapReport.java`

- How to de-obfuscate the report？

    - Use proguard ReTrace