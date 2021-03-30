[![license](https://img.shields.io/badge/license-Apache--2.0-brightgreen.svg)](https://github.com/KwaiAppTeam/KOOM/blob/master/LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Android-brightgreen.svg)](https://github.com/KwaiAppTeam/KOOM/wiki/home)
# KOOM
An OOM killer on mobile platform by Kwai. 

中文版本请参看[这里](README.zh-CN.md)

## Introduction

KOOM creates a mobile high performance online memory monitoring solution，which supplies a detailed report when OOM related problems are detected, and has solved a large number of OOM issues in the Kwai application. It's currently available on **Android**.

## Highlights

### High Performance
KOOM blocks the application less than 100ms by forking child process to dump hprof, it also has an efficient leak detect module and a fast hprof analysis module.

### High Reliability
KOOM's performance and stability have withstood the test of hundreds of millions of  number of users and devices.

### Less Code
You just need to init KOOM,  and it will take care of other things for you. Advanced custom config is also supported.


## Getting started

### First look of koom-demo

Try to run the koom-demo project first, and have a general understanding of the functionality provided by KOOM.

### Gradle dependencies

```gradle
dependencies {
    implementation 'com.kwai.koom:java-oom:1.1.0'
}
```

### Quick Tutorial
You can setup KOOM as soon as you want to start memory monitoring, to setup on App startup, you can do like this:

```Java
public class KOOMApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        KOOM.init(this);
    }

}
```

### Java-oom Heap Report

Heap dump and analysis is executed automatically.

When the java heap is over the threshold by heap monitor, the heap dump and heap analysis is triggered then.

A java-oom heap report will be generated when heap analysis done.

Find a time to get the report manually.
```Java
public void getReportManually() {
    File reportDir = new File(KOOM.getInstance().getReportDir());
    for (File report : reportDir.listFiles()) {
        // Upload the report or do something else.
    }
}
```

Or set a listener to listen and get the report file status.
```Java
public void listenReportGenerateStatus() {
    KOOM.getInstance().setHeapReportUploader(file -> {
        // Upload the report or do something else.
        // File is deleted automatically when callback is done by default.
    });
}
```

### JAVA8 Requirements
```gradle
compileOptions {
        sourceCompatibility JavaVersion.VERSION_1_8
        targetCompatibility JavaVersion.VERSION_1_8
    }
```

### Custom Feature

See wiki [Advanced Custom Feature](https://github.com/KwaiAppTeam/KOOM/wiki/Advanced-Custom-Feature)

### Compatibility

See wiki [Compatibility](https://github.com/KwaiAppTeam/KOOM/wiki/Compatibility)

### FAQ

See wiki [FAQ](https://github.com/KwaiAppTeam/KOOM/wiki/FAQ)

## Performance
Randomly dump hprof of real users online, and the time consumed by normal dump and for dump blocking users is as follows:

<img src="https://github.com/KwaiAppTeam/KOOM/wiki/images/android_benchmark.png" width="500">

For more detail, please refer to [our benchmark](https://github.com/KwaiAppTeam/KOOM/wiki/android_benchmark).

## License
KOOM is under the Apache license 2.0. For details check out the [LICENSE](./LICENSE).

## Change Log
Check out the [CHANGELOG.md](./CHANGELOG.md) for details of change history.

## Contributing
If you are interested in contributing, check out the [CONTRIBUTING.md](./CONTRIBUTING.md)

## Feedback
Welcome report [issues](https://github.com/KwaiAppTeam/KOOM/issues) or contact us in WeChat group.

<img src=./doc/images/wechat.jpg/>