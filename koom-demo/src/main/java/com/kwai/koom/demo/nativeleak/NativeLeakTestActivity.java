package com.kwai.koom.demo.nativeleak;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import com.kwai.koom.base.MonitorLog;
import com.kwai.koom.base.MonitorManager;
import com.kwai.koom.base.Monitor_ProcessKt;
import com.kwai.koom.base.Monitor_SoKt;
import com.kwai.koom.demo.R;
import com.kwai.koom.nativeoom.leakmonitor.LeakListener;
import com.kwai.koom.nativeoom.leakmonitor.LeakMonitor;
import com.kwai.koom.nativeoom.leakmonitor.LeakMonitorConfig;
import com.kwai.koom.nativeoom.leakmonitor.LeakRecord;

import java.util.Collection;
import java.util.Collections;

public class NativeLeakTestActivity extends AppCompatActivity {
  private static final String LOG_TAG = "NativeLeakTestActivity";

  public static void start(Context context) {
    context.startActivity(new Intent(context, NativeLeakTestActivity.class));
    if (!Monitor_SoKt.loadSoQuietly("native-leak-test")) {
      throw new RuntimeException("test so load fail");
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_native_leak_test);
    initLeakMonitor();
    findViewById(R.id.btn_start_monitor).setOnClickListener(
        view -> LeakMonitor.INSTANCE.start()
    );

    findViewById(R.id.btn_trigger_leaks).setOnClickListener(
        view -> NativeLeakTest.triggerLeak(new Object())
    );

    findViewById(R.id.btn_check_leaks).setOnClickListener(
        view -> LeakMonitor.INSTANCE.checkLeaks()
    );

    findViewById(R.id.btn_stop_monitor).setOnClickListener(
        (view) -> LeakMonitor.INSTANCE.stop()
    );
  }

  private void initLeakMonitor() {
    if (!Monitor_ProcessKt.isMainProcess() || !Monitor_ProcessKt.isArm64()) {
      MonitorLog.e(LOG_TAG, "Only Main Process and Arm64 can run LeakMonitor");
      return;
    }

    if (LeakMonitor.INSTANCE.isInitialized()) {
      return;
    }

    LeakMonitorConfig config = new LeakMonitorConfig.Builder()
        .setLoopInterval(50000) // 设置轮训的间隔
        .setLeakItemThreshold(200) // 收集泄漏的native对象的上限
        .setMonitorThreshold(16) // 设置监听的最小内存值
        .setNativeHeapAllocatedThreshold(0) // 设置native heap分配的内存达到多少阈值开始监控
        .setSelectedSoList(new String[0]) // 不设置是监控所有， 设置是监听特定的so,  比如监控libcore.so 填写 libcore 不带.so
        .setIgnoredSoList(new String[0]) // 设置需要忽略监控的so
        .setLeakListener(leaks -> {
          if (leaks.isEmpty()) {
            return;
          }
          StringBuilder builder = new StringBuilder();
          for (LeakRecord leak : leaks) {
            builder.append(leak.toString());
          }
          Toast.makeText(this, builder.toString(), Toast.LENGTH_SHORT).show();
        })
        .build();
    MonitorManager.addMonitorConfig(config);
  }
}