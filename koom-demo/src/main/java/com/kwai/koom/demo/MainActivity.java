package com.kwai.koom.demo;

import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

import com.kwai.koom.base.Monitor_SoKt;
import com.kwai.koom.demo.leaked.LeakMaker;
import com.kwai.koom.demo.nativeleak.NativeLeakTest;
import com.kwai.koom.javaoom.KOOM;
import com.kwai.koom.javaoom.KOOMProgressListener;
import com.kwai.koom.nativeoom.leakmonitor.LeakMonitor;

import java.util.concurrent.TimeUnit;

/**
 * Copyright 2020 Kwai, Inc. All rights reserved.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
public class MainActivity extends AppCompatActivity {

  private Button reportButton;
  private TextView reportText;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    reportButton = findViewById(R.id.btn_report_leak);
    reportText = findViewById(R.id.tv_report_status);

    findViewById(R.id.btn_report_leak).setOnClickListener(v -> {
      reportButton.setVisibility(View.GONE);
      reportText.setVisibility(View.VISIBLE);

      LeakMaker.makeLeak(MainActivity.this);

      testReport();
    });

    findViewById(R.id.btn_test_native_leak).setOnClickListener(v -> {
      if (Monitor_SoKt.loadSoQuietly("native-leak-test")) {
        LeakMonitor.INSTANCE.startLoop(true, false, 0);
        try {
          TimeUnit.MILLISECONDS.sleep(500);
        } catch (InterruptedException interruptedException) {
          interruptedException.printStackTrace();
        }
        NativeLeakTest.triggerLeak(new Object());
      }
    });

  }

  private void changeStatusText(KOOMProgressListener.Progress progress) {
    mainHandler.post(() -> chanStatusTextInMain(progress));
  }

  private void chanStatusTextInMain(KOOMProgressListener.Progress progress) {
    String text = "";
    switch (progress) {
      case HEAP_DUMP_START:
        text = "heap dump started";
        break;
      case HEAP_DUMPED:
        text = "heap dump ended";
        break;
      case HEAP_DUMP_FAILED:
        text = "heap dump failed";
        break;
      case HEAP_ANALYSIS_START:
        text = "heap analysis start";
        break;
      case HEAP_ANALYSIS_DONE:
        text = "heap analysis done, please check report in " + KOOM.getInstance().getReportDir();
        break;
      case HEAP_ANALYSIS_FAILED:
        text = "heap analysis failed";
        break;
      default:
        break;
    }
    reportText.setText(text);
  }

  private Handler mainHandler;
  private static final String TAG = "KOOM";

  public void testReport() {
    mainHandler = new Handler();
    mainHandler.postDelayed(() -> {
      KOOM.getInstance().manualTrigger();
      KOOM.getInstance().setProgressListener(this::changeStatusText);
    }, 1500);
  }

}