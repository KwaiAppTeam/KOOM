# LeakMonitor 介绍

用于监控应用的 Native 内存泄漏问题，它的核心原理
- hook malloc/free 等内存分配器方法，用于记录 Native 内存分配元数据「大小、堆栈、地址等」
- 周期性的使用 mark-and-sweep 分析整个进程 Native Heap，获取不可达的内存块信息「地址、大小」
- 利用不可达的内存块的地址、大小等从我们记录的元数据中获取其分配堆栈，产出泄漏数据「不可达内存块地址、大小、分配堆栈等」

# LeakMonitor 适用范围
- Android N 及以上（API level >= 24）
- 仅支持 arm64-v8a

# LeakMonitor 接入
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
    implementation "com.kuaishou.koom:koom-native-leak:${latest_version}"
    implementation "com.kuaishou.koom:xhook:${latest_version}"
}
```
## 使用
- 初始化 MonitorManager, 参考[这里](../koom-monitor-base/README.zh-CN.md)
由于 LeakMonitor 依赖 MonitorManager，确保 MonitorManager 已经初始化

- 初始化 LeakMonitor
```java
LeakMonitorConfig config = new LeakMonitorConfig.Builder()
    .setLoopInterval(50000) // 设置轮训的间隔，单位：毫秒
    .setMonitorThreshold(16) // 设置监听的最小内存值，单位：字节
    .setNativeHeapAllocatedThreshold(0) // 设置native heap分配的内存达到多少阈值开始监控，单位：字节
    .setSelectedSoList(new String[0]) // 不设置是监控所有， 设置是监听特定的so,  比如监控libcore.so 填写 libcore 不带.so
    .setIgnoredSoList(new String[0]) // 设置需要忽略监控的so
    .setEnableLocalSymbolic(false) // 设置使能本地符号化，仅在 debuggable apk 下有用，release 请关闭
    .setLeakListener(leaks -> { }) // 设置泄漏监听器
    .build();
MonitorManager.addMonitorConfig(config);
```
- 启动 LeakMonitor，开始周期性的检测泄漏
```java
LeakMonitor.INSTANCE.start();
```
- 停止 LeakMonitor，通常不用主动停止
```java
LeakMonitor.INSTANCE.stop();
```
- 主动获取泄漏，在`LeakListener`中接收到泄漏信息；通常不需要主动检查
```java
LeakMonitor.INSTANCE.checkLeaks();
```
# FAQ
- 为什么不支持 Android N 以下的设备？
    - AOSP 在 Android N 之后系统才增加了 libmemunreachable 模块「当然也可以自己抽出来在 APP 测实现」
    - 考虑到内存泄漏的共性，只监控 Android N 以上的设备应该可解决大多数问题
- 为什么不支持 armeabi-v7a
    - 获取内存分配堆栈时，出于性能的考虑我们使用了 FP Unwind(frame pointer unwind)；而 armeabi-v7a 采用 ARM/Thumb 混合指令，而 FP Unwind 在 ARM/Thumb 指令混合的方法栈上不可靠
    - AArch32(Arm 32-bit Architecture) 的 ABI 也没有规定 FP 的行为
- LeakMonitor 的性能开销如何？
    - 由于需要获取、保存内存块的堆栈等元数据信息，性能有一定的开销，根据我们的测试整体性能损耗 < 5%，基本上不影响用户体验
    - 线上使用强烈建议在「性能较好的设备上采样」开启
- 检测到的泄漏问题 100% 是泄漏吗？
    - 根据检测原理及经验，泄漏检测准确率基本上可以达到 90%+，可能有个别 bad case 存在
        - 检测采用不精确的 mark-and-sweep 算法，分析时总是按照 8 字节进行可达性分析，不是 100% 精确
        - 由于内存分配器脏内存的影响，可能导致某些泄漏内存是可达的，影响结果