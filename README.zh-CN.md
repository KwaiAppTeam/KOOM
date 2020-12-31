# KOOM——高性能线上内存监控方案
KOOM(Kwai OOM, Kill OOM)是快手性能优化团队在处理移动端OOM问题的过程中沉淀出的一套完整解决方案。

其中Android Java内存部分在LeakCanary的基础上进行了大量优化，解决了线上内存监控的性能问题，在不影响用户体验的前提下线上采集内存镜像并解析。从 2020 年春节后在快手主APP上线至今解决了大量OOM问题，其性能和稳定性经受住了海量用户与设备的考验，因此决定开源以回馈社区，并欢迎大家来帮助我们改进。

## KOOM 背景
随着移动端业务逻辑日益复杂，4K编解码、AR魔表等高内存需求场景的逐渐普及，OOM问题已成为快手客户端稳定性治理的头号顽疾。  在日常版本迭代过程中，间或会发生OOM激增，而线上环境非常复杂，仅AB实验就有上千种，事前预防以及事后还原都无法做到，因此急需高性能的线上内存监控方案。一期开源的Android Java内存监控方案，我们调研了LeakCanary以及美团和UC等发表的相关技术文章，发现业内的优化方向主要集中在内存镜像的解析部分，而内存镜像dump部分，一直没有方案能解决dump过程中app长时间冻结的问题。经过深入研究，我们发现可以利用**Copy-on-write**机制fork子进程dump，满足我们的需求。


## KOOM 指南

### 运行demo

首先建议跑通一下项目自带demo(koom-demo项目)，对KOOM提供的基础功能和概念有一个大致的了解。

### 依赖接入

```gradle
dependencies {
    implementation 'com.kwai.koom:java-oom:1.0.7'
}
```

### 初始化
Application初始化

```Java
public class KOOMApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        KOOM.init(this);
    }

}
```

### Java-oom 报告获取
当内存使用异常，触发内存镜像采集并分析后，会生成一份json格式的报告。

可以择机主动获取
```Java
public void getReportManually() {
    File reportDir = new File(KOOM.getInstance().getReportDir());
    for (File report : reportDir.listFiles()) {
        // Upload the report or do something else.
    }
}
```

也可以实时监听报告生成状态
```Java
public void listenReportGenerateStatus() {
    KOOM.getInstance().setHeapReportUploader(file -> {
        // Upload the report or do something else.
        // File is deleted automatically when callback is done by default.
    });
}
```

### JAVA8 要求
```gradle
compileOptions {
        sourceCompatibility JavaVersion.VERSION_1_8
        targetCompatibility JavaVersion.VERSION_1_8
    }
```

### 自定义需求

查看wiki [Advanced Custom Feature](https://github.com/KwaiAppTeam/KOOM/wiki/Advanced-Custom-Feature)

### Compatibility

See wiki [Compatibility](https://github.com/KwaiAppTeam/KOOM/wiki/Compatibility)

### FAQ

See wiki [FAQ](https://github.com/KwaiAppTeam/KOOM/wiki/FAQ)

### 镜像采集性能对比
随机采集线上真实用户的内存镜像，普通dump和fork子进程dump阻塞用户使用的耗时如下：

<img src="https://github.com/KwaiAppTeam/KOOM/wiki/images/android_benchmark_cn.png" width="500">

想了解更多细节请参考 [Android Benchmark](https://github.com/KwaiAppTeam/KOOM/wiki/android_benchmark_cn)。

### Java-oom 组件介绍
* **内存监控组件**
定时采集内存资源占用情况，超过阈值触发内存镜像采集，决定镜像dump与分析时机，关键代码参考`Monitor.java`

* **内存镜像采集组件**
高性能内存镜像采集组件，包含fork dump和strip dump两个部分，关键代码参考`HeapDumper.java`

* **内存镜像解析组件**
高性能内存镜像解析组件，基于shark解析器定制优化，泄露判定关键代码参考`LeakDetector.java`

## License

KOOM 以 Apache-2.0 证书开源，详情参见 [LICENSE](./LICENSE)。

## 版本历史
具体版本历史请参看 [CHANGELOG.md](./CHANGELOG.md)。

## 参与贡献
如果你有兴趣参与贡献，可以参考 [CONTRIBUTING.md](./CONTRIBUTING.md)。


## 问题 & 反馈
欢迎提 [issues](https://github.com/KwaiAppTeam/KOOM/issues) 提问反馈。

## 联系我们

**项目负责人**
[alhah(薛秋实)](https://github.com/alhah)

**项目核心成员**
[alhah(薛秋实)](https://github.com/alhah)
[AndroidInternal(李锐)](https://github.com/AndroidInternal)
**微信讨论群**
<img src=./doc/images/wechat.jpg/>。

