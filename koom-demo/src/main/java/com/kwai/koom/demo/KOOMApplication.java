package com.kwai.koom.demo;

import java.io.File;

import android.app.Application;

import com.kwai.koom.javaoom.KOOM;
import com.kwai.koom.javaoom.common.KConfig;
import com.kwai.koom.javaoom.common.KLog;
import com.kwai.koom.javaoom.dump.ForkJvmHeapDumper;

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
public class KOOMApplication extends Application {

  @Override
  public void onCreate() {
    super.onCreate();
    KOOM.init(this);
  }

  //Example of how to get report manually.
  public void getReportManually() {
    File reportDir = new File(KOOM.getInstance().getReportDir());
    for (File report : reportDir.listFiles()) {
      //Upload the report or do something else.
    }
  }

  //Example of how to listen report's generate status.
  public void listenReportGenerateStatus() {
    KOOM.getInstance().setHeapReportUploader(file -> {
      //Upload the report or do something else.
      //File is deleted automatically when callback is done by default.
    });
  }

  //Example of how to set custom config.
  public void customConfig() {
    KConfig kConfig = new KConfig.KConfigBuilder()
            .heapRatio(85.0f) //heap occupied ration in percent, 85.0f means use 85% memory of max heap
            .rootDir(this.getCacheDir().getAbsolutePath()) //root dir stores report and hprof files
            .heapOverTimes(3) //heap max times of over heap's used threshold
            .build();
    KOOM.getInstance().setKConfig(kConfig);
  }

  //Example of how to set custom logger.
  public void customLogger() {
    KOOM.getInstance().setLogger(new KLog.KLogger() {
      @Override
      public void i(String TAG, String msg) {
        //get the log of info level
      }

      @Override
      public void d(String TAG, String msg) {
        //get the log of debug level
      }

      @Override
      public void e(String TAG, String msg) {
        //get the log of error level
      }
    });
  }

  //Example of set custom koom root dir.
  public void customRootDir() {
    //Be careful with case when res is false which means dir is not valid.
    boolean res = KOOM.getInstance().setRootDir(this.getCacheDir().getAbsolutePath());
  }

  //Example of dump hprof directly
  public void customDump() {
    //Same with StandardHeapDumper StripHprofHeapDumper
    new ForkJvmHeapDumper().dump("absolute-path");
  }

}
