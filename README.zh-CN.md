# KOOM——高性能线上内存监控方案
KOOM(Kwai OOM, Kill OOM)是快手性能优化团队在处理移动端OOM问题的过程中沉淀出的一套完整解决方案。

随着移动端业务逻辑日益复杂，4K编解码、AR魔表等高内存需求场景的逐渐普及，OOM问题已成为快手客户端稳定性治理的头号顽疾。在日常版本迭代过程中，间或会发生OOM激增，而线上环境非常复杂，仅AB实验就有上千种，事前预防以及事后还原都无法做到，因此急需高性能的线上内存监控方案。

那么 OOM 治理应该如何建设呢？目前 KOOM 已经具备了 Java Heap/Native Heap/Thread 泄漏监控能力，后续还会建设更多维度、场景监控


## KOOM 功能
### Java Heap 泄漏监控
- `koom-java-leak` 模块用于 Java Heap 泄漏监控：它利用 Copy-on-write 机制 fork 子进程 dump Java Heap，解决了 
  dump 过程中 app 长时间冻结的问题，详情参考 [这里](./koom-java-leak/README.zh-CN.md)
### Native Heap 泄漏监控
- `koom-native-leak` 模块用于 Native Heap 泄漏监控：它利用 [Tracing garbage collection](https://en.wikipedia.org/wiki/Tracing_garbage_collection) 
机制分析整个 Native Heap，直接输出泄漏内存信息「大小、分配堆栈等』；极大的降低了业务同学分析、解决内存泄漏的成本。详情可以参考 [这里](./koom-native-leak/README.zh-CN.md)
### Thread 泄漏监控
- `koom-thread-leak` 模块用于 Thread 泄漏监控：它会 hook 线程的生命周期函数，周期性的上报泄漏线程信息。详情参考 [这里](./koom-thread-leak/README.zh-CN.md)

## STL 支持
所有Native模块均支持c++_shared和c++_static两种接入模式，详情参考 [cpp-support](https://developer.android.com/ndk/guides/cpp-support)
- 项目 build.gradle 中增加依赖(以koom-java-leak为例)：
```groovy
dependencies {
  // shared模式，多个模块共享同一个libc++_shared.so，包体较小，但当多个模块依赖的STL版本不同时，最终编译会发生冲突。
  // 例如, 可能会遇到 "dlopen failed: cannot locate symbol "__emutls_get_address" referenced by" 错误.
  implementation "com.kuaishou.koom:koom-java-leak:${latest_version}"
  // or static模式，包体较大，无编译和运行时问题。
  implementation "com.kuaishou.koom:koom-java-leak-static:${latest_version}"
  // 如果依赖多个模块，shared和static模式不能混用，如下写法是错误的，切记！
  implementation "com.kuaishou.koom:koom-java-leak-static:${latest_version}"
  implementation "com.kuaishou.koom:koom-monitor-base:${latest_version}"
}
```
- 介绍一种解决shared模式冲突的办法，项目根目录 build.gradle 中增加pickFirst：
```groovy
packagingOptions {
  // apk打包时选择第一个libc++_shared.so，运行时可能遇到不可预知的bug，慎用！
  pickFirst 'lib/*/libc++_shared.so'
}
```

## minSdk
- 所有模块编译时的minSdk为18，如果接入方的minSdk低于此值，需要在manifest中通过overrideLibrary兼容：
```xml
  <uses-sdk tools:overrideLibrary="com.kwai.koom.fastdump, com.kwai.android.base, com.kwai.koom.base" />
```

## License

KOOM 以 Apache-2.0 证书开源，详情参见 [LICENSE](./LICENSE)。

## 版本历史
具体版本历史请参看 [CHANGELOG.md](./CHANGELOG.md)。

## 参与贡献
如果你有兴趣参与贡献，可以参考 [CONTRIBUTING.md](./CONTRIBUTING.md)。


## 问题 & 反馈
欢迎提 [issues](https://github.com/KwaiAppTeam/KOOM/issues) 提问反馈。

## 联系我们

**项目负责人**<br>
[alhah(薛秋实)](https://github.com/alhah) <br>

**项目核心成员**<br>
[alhah(薛秋实)](https://github.com/alhah) <br>
[AndroidInternal(李锐)](https://github.com/AndroidInternal) <br>
[lbtrace(王连宝)](https://github.com/lbtrace) <br>
[shenvsv(沈冠初)](https://github.com/shenvsv) <br>

**微信讨论群** <br>
<img src=./doc/images/wechat.jpg/>。
