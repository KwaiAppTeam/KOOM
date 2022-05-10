package com.kwai.koom.demo.javaleak;

import java.io.File;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.kwai.koom.demo.MainActivity;
import com.kwai.koom.demo.R;
import com.kwai.koom.demo.javaleak.test.LeakMaker;
import com.kwai.koom.javaoom.hprof.ForkStripHeapDumper;
import com.kwai.koom.javaoom.monitor.OOMMonitor;

public class JavaLeakTestActivity extends AppCompatActivity {

  public static void start(Context context) {
    context.startActivity(new Intent(context, JavaLeakTestActivity.class));
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_java_leak_test);
  }

  @SuppressLint("NonConstantResourceId")
  public void onClick(View v) {
    switch (v.getId()) {
      case R.id.btn_make_java_leak:
        showJavaLeakHint();

        /*
         * Init OOMMonitor
         */
        OOMMonitorInitTask.INSTANCE.init(JavaLeakTestActivity.this.getApplication());
        OOMMonitor.INSTANCE.startLoop(true, false,5_000L);

        /*
         * Make some leaks for test!
         */
        LeakMaker.makeLeak(this);
        break;

      case R.id.btn_hprof_dump:
        showHprofDumpHint();

        //Pull the hprof from the devices.
        //adb shell "run-as com.kwai.koom.demo cat 'files/test.hprof'" > ~/temp/test.hprof
        ForkStripHeapDumper.getInstance().dump(
            this.getFilesDir().getAbsolutePath() + File.separator + "test.hprof");
        break;
    }
  }

  private void showJavaLeakHint() {
    findViewById(R.id.btn_make_java_leak).setVisibility(View.INVISIBLE);
    findViewById(R.id.btn_hprof_dump).setVisibility(View.INVISIBLE);
    findViewById(R.id.tv_make_java_leak_hint).setVisibility(View.VISIBLE);
  }

  private void showHprofDumpHint() {
    findViewById(R.id.btn_make_java_leak).setVisibility(View.INVISIBLE);
    findViewById(R.id.btn_hprof_dump).setVisibility(View.INVISIBLE);
    findViewById(R.id.tv_hprof_dump_hint).setVisibility(View.VISIBLE);
  }
}
