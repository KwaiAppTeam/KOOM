# OOMMonitor 介绍

用于监控应用的 Java 内存泄漏问题，它的核心原理

- 周期性查询Java堆内存、线程数、文件描述符数等资源占用情况，当连续多次超过设定阈值或突发性连续快速突破高阈值时，触发镜像采集

- 镜像采集采用`虚拟机supend->fork虚拟机进程->虚拟机resume->dump内存镜像`的策略，将传统Dump冻结进程20s的时间缩减至20ms以内

- 基于shark执行镜像解析，并针对shark做了一系列调整用于提升性能，在手机设备测即可执行离线内存泄露判定与引用链查找，生成分析报告



# OOMMonitor 适用范围

- Android L 及以上（API level >= 21）

- 支持 armeabi-v7a arm64-v8a x86 x86-64



# OOMMonitor 接入

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
  implementation "com.kuaishou.koom:koom-java-leak:${latest_version}"
}
```

## 使用

- 初始化 MonitorManager

由于 OOMMonitor 依赖 MonitorManager，确保 MonitorManager 已经初始化，如Application、版本号等已正确传参

- 初始化 OOMMonitor

支持自定义诸多参数并获取分析报告，具体参考代码：

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

- 启动 OOMMonitor，开始周期性的检测泄漏

```kotlin
OOMMonitor.startLoop(5_000L)
```

-  停止 OOMMonitor，通常不用主动停止

```kotlin
OOMMonitor.stopLoop()
```

- 日志检验集成效果
  - 开启轮询日志，`OOMMonitor: startLoop()`
  - 超过阈值日志，`OOMMonitor_ThreadOOMTracker: [meet condition] overThresholdCount:1, threadCount: 717`
  - 触发Dump分析日志，`OOMMonitor: dumpAndAnalysis`
  - Dump完成日志：`OOMMonitor: end hprof dump`
  - 开始分析日志：`OOMMonitor: start hprof analysis`
  - 分析完成日志：`OOMMonitor: heap analysis success, do upload`

# OOMMonitor FAQ

- OOMMonitor 的性能开销如何？
  - Dump镜像不再阻塞主进程的运行状态，此开销可以忽略
  - 子进程Dump镜像及分析的耗时，根据镜像大小最大可达3分钟以上，占用一个线程，内存开销通常在200M以内
  - 综上，建议使用者建立远程开关机制，采样开启

- 泄露是如何判定的？
  - 对于Activity和Fragment当对象已经销毁却仍有从GC Root到此对象的引用路径时，认为此对象已经泄露
    - Activity的销毁判定规则为，mFinished或mDestroyed值为true
    - Fragment的销毁判定规则为，mFragmentManager为空且mCalled为true，mCalled为true表示此fragment已经经历了一些生命周期

- 如何裁剪镜像？

    - ```java
      ForkStripHeapDumper.getInstance().dump(path)
      ```

- 裁剪的镜像如何恢复，使得AS Profiler/MAT能够打开？

  - 取出裁剪镜像

    - ```shell
      adb shell "run-as com.kwai.koom.demo cat 'files/test.hprof'" > ~/temp/test.hprof
      ```

  - 使用`tools/koom-fill-crop.jar`恢复裁剪镜像

    - ```shell
      java -jar koom-fill-crop.jar test.hprof
      ```

- 分析报告里的引用链如何理解？

  - 首先，可以查看`HeapReport.java`里的注释
  - reference：除数组外，均由类名 + 字段名组合而成，其中字段属于此类中含有的字段，引用链的引用关系为：引用链上一层的对象持有了此字段的实例或静态字段声明的引用
  - referenceType：代表引用类型，可以分别为：
    - INSTANCE_FIELD：代表引用的是普通字段的实例，表现形式通常为通过等号赋值new出来的对象
    - ARRARY_ENTRY：代表引用方式为数组中某个index存储对象的实例
    - STATIC_FIELD：代表引用的是类中声明的静态字段
  - declaredClass：字段不一定是在类中直接声明的，字段也有可能是从父类中继承的，declaredClass代表了此字段具体是取自那个类。

- 分析报告里混淆的类如何解析、聚合？

  - 此部分代码没有开源，请和每个公司的APM后台相结合
  - 反混淆可以使用progurad的ReTrace功能