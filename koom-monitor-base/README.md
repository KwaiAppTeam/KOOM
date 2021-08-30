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
## Code usage
- Define initialization tasks
```kotlin
object CommonInitTask : InitTask {
  override fun init(application: Application) {
    val config = CommonConfig.Builder()
      .setApplication(application) // Set application
      .setVersionNameInvoker { "1.0.0" } // Set version name, java leak feature use it
      .build()

    MonitorManager.initCommonConfig(config)
      .apply { onApplicationCreate() }
  }
}
```
- Initialize `MonitorManager` in the `onCreate` method of `Application`
```java
public class KOOMApplication extends Application {

  @Override
  public void onCreate() {
    super.onCreate();
    CommonInitTask.INSTANCE.init(this);
  }
}
```