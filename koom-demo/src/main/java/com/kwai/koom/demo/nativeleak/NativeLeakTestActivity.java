package com.kwai.koom.demo.nativeleak;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.widget.Toast;

import com.kwai.koom.base.MonitorBuildConfig;
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
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N
        || !Monitor_ProcessKt.isMainProcess()
        || !Monitor_ProcessKt.isArm64()) {
      MonitorLog.e(LOG_TAG, "Only Main Process and Arm64 can run LeakMonitor");
      Toast.makeText(this, "LeakMonitor NOT work!! Check OS Version/CPU ABI",
          Toast.LENGTH_SHORT).show();
      return;
    }
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
    if (LeakMonitor.INSTANCE.isInitialized()) {
      return;
    }

    LeakMonitorConfig config = new LeakMonitorConfig.Builder()
        .setLoopInterval(50000) // Set polling interval, time unit: millisecond
        .setMonitorThreshold(16) // Set the threshold of the monitored memory block, unit: byte
        .setNativeHeapAllocatedThreshold(0) // Set the threshold of how much memory allocated by
        // the native heap reaches to start monitoring, unit: byte
        .setSelectedSoList(new String[0]) // Set the monitor specific libraries, such as monitoring libcore.so, just write 'libcore'
        .setIgnoredSoList(new String[0]) // Set the libraries that you need to ignore monitoring
        .setEnableLocalSymbolic(false) // Set enable local symbolic, this is helpful in debug
        // mode. Not enable in release mode
        .setLeakListener(leaks -> { // Set Leak Listener for receive Leak info
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