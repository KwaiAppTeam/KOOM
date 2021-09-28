[![license](https://img.shields.io/badge/license-Apache--2.0-brightgreen.svg)](https://github.com/KwaiAppTeam/KOOM/blob/master/LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Android-brightgreen.svg)](https://github.com/KwaiAppTeam/KOOM/wiki/home)
# KOOM
An OOM killer on mobile platform by Kwai. 

中文版本请参看[这里](README.zh-CN.md)

## Introduction

KOOM creates a mobile high performance online memory monitoring solution，which supplies a detailed report when OOM related problems are detected, and has solved a large number of OOM issues in the Kwai application. It's currently available on **Android**.

With the increasing complexity of mobile terminal business logic and the gradual popularity of scenarios with high memory requirements such as 4K codec and AR magic watch, the OOM problem has become the number one problem in the stability management of the Kuaishou client. 
In the daily version iteration process, OOM surges occasionally occur, and the online environment is very complicated. There are thousands of AB experiments. Pre-prevention and post-recovery cannot be achieved. Therefore, high-performance online memory monitoring solutions are urgently needed.

So how should OOM governance be built? At present, KOOM has the capability of monitoring leakage of Java Heap/Native Heap/Thread, and will build multi-dimensional and multi-business scenarios monitoring in the future.

## Features

### Java Leak Monitor
- The `koom-java-leak` module is used for Java Heap leak monitoring: it uses the Copy-on-write 
mechanism to fork the child process dump Java Heap, which solves the problem.
The app freezes for a long time during the dump. For details, please refer to [here](./koom-java-leak/README.md)
### Native Leak Monitor
- The `koom-native-leak` module is a Native Heap leak monitoring solution: use the [Tracing garbage collection](https://en.wikipedia.org/wiki/Tracing_garbage_collection) mechanism to analyze the entire Native Heap, and directly output the leaked memory information like: size/Allocating stacks/etc.; 
  greatly reduces the cost of analyzing and solving memory leaks for business students. For details, please refer to [here](./koom-native-leak/README.md)
### Thread Leak Monitor
- The `koom-thread-leak` module is used for Thread leak monitoring: it hooks the life cycle 
  function of the thread, and periodically reports the leaked thread information. For details, please refer to [here](./koom-thread-leak/README.md)

## License
KOOM is under the Apache license 2.0. For details check out the [LICENSE](./LICENSE).

## Change Log
Check out the [CHANGELOG.md](./CHANGELOG.md) for details of change history.

## Contributing
If you are interested in contributing, check out the [CONTRIBUTING.md](./CONTRIBUTING.md)

## Feedback
Welcome report [issues](https://github.com/KwaiAppTeam/KOOM/issues) or contact us in WeChat group.

<img src=./doc/images/wechat.jpg/>