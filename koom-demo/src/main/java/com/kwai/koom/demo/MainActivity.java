/*
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

package com.kwai.koom.demo;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

import com.kwai.koom.base.Monitor_SoKt;
import com.kwai.koom.demo.javaleak.JavaLeakTestActivity;
import com.kwai.koom.demo.javaleak.test.LeakMaker;
import com.kwai.koom.demo.nativeleak.NativeLeakTestActivity;
import com.kwai.koom.demo.threadleak.ThreadLeakTest;
import com.kwai.koom.demo.threadleak.ThreadLeakTestActivity;

public class MainActivity extends AppCompatActivity {


  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    findViewById(R.id.btn_java_leak).setOnClickListener(v -> {
      JavaLeakTestActivity.start(MainActivity.this);
    });

    findViewById(R.id.btn_test_native_leak).setOnClickListener(v -> {
      NativeLeakTestActivity.start(MainActivity.this);
    });

    findViewById(R.id.btn_test_thread_leak).setOnClickListener(v -> {
      ThreadLeakTestActivity.Companion.start(MainActivity.this);
    });
  }
}