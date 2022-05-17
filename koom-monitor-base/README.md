# Monitor base Introduction
The basic module shared by the koom monitoring function. Before all monitoring modules are initialized, please make sure that the module has been initialized

# Monitor base Get Started
## Setup dependencies
- Add mavenCentral to the repositories of the project root directory build.gradle
```groovy
repositories {
    mavenCentral()
}
```

- Add dependency in project app/build.gradle
```groovy
dependencies {
    implementation "com.kuaishou.koom:koom-monitor-base:${latest_version}"
}
```
## Setup code
- Initialize `MonitorManager` in the `onCreate` method of `Application`
```java
public class KOOMApplication extends Application {

  @Override
  public void onCreate() {
    super.onCreate();
    DefaultInitTask.INSTANCE.init(this);
  }
}
```