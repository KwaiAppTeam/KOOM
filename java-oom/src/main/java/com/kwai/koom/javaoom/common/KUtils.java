package com.kwai.koom.javaoom.common;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.os.StatFs;
import android.text.TextUtils;

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
public class KUtils {

  private static long startupTime;

  public static void startup() {
    startupTime = System.currentTimeMillis();
  }

  public static int usageSeconds() {
    return (int) ((System.currentTimeMillis() - startupTime) / 1000);
  }

  public static String getTimeStamp() {
    return new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss",
        Locale.CHINESE).format(new Date());
  }

  public static float getSpaceInGB(String dir) {
    StatFs statFs = new StatFs(dir);
    long blockSize = statFs.getBlockSizeLong();
    long availableBlocks = statFs.getAvailableBlocks();
    return 1.0f * blockSize * availableBlocks / KConstants.Bytes.GB;
  }

  public static int computeGenerations(Class<?> clazz) {
    int generation = 1;
    while (clazz != null && clazz.getSuperclass() != Object.class) {
      clazz = clazz.getSuperclass();
      generation++;
    }
    assert clazz != null;
    return generation;
  }

  private static String sProcessName;

  public static String getProcessName() {
    if (!TextUtils.isEmpty(sProcessName)) {
      return sProcessName;
    }
    try {
      int pid = android.os.Process.myPid();
      ActivityManager manager =
          (ActivityManager) KGlobalConfig.getApplication()
              .getSystemService(Context.ACTIVITY_SERVICE);
      if (manager != null) {
        List<ActivityManager.RunningAppProcessInfo> appProcessList =
            manager.getRunningAppProcesses();
        if (appProcessList != null) {
          for (ActivityManager.RunningAppProcessInfo processInfo : appProcessList) {
            if (processInfo.pid == pid) {
              sProcessName = processInfo.processName;
              break;
            }
          }
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    // 增加兜底https://stackoverflow.com/questions/19631894/is-there-a-way-to-get-current-process
    // -name-in-android
    if (TextUtils.isEmpty(sProcessName)) {
      try (BufferedReader cmdlineReader = new BufferedReader(new InputStreamReader(
          new FileInputStream("/proc/self/cmdline")))) {
        // 可能并发访问，不要用StringBuilderHolder.getGlobal()
        StringBuilder processName = new StringBuilder();
        int c;
        while ((c = cmdlineReader.read()) > 0) {
          processName.append((char) c);
        }
        sProcessName = processName.toString();
      } catch (IOException e) {
        e.printStackTrace();
      }
    }
    return sProcessName;
  }

  public static class ProcessStatus {
    public long totalByteSize;
    public long vssKbSize;
    public long rssKbSize;
    public long pssKbSize;
    public long javaHeapByteSize;
    public int threadsCount;
  }

  /**
   * 获取进程的Vss，Rss等状态
   */
  public static ProcessStatus getProcessMemoryUsage() {
    ProcessStatus usage = new ProcessStatus();
    RandomAccessFile reader = null;
    try {
      reader = new RandomAccessFile("/proc/self/status", "r");

      String line;

      while ((line = reader.readLine()) != null) {
        if (TextUtils.isEmpty(line)) {
          continue;
        }

        if (line.startsWith("VmSize") && line.contains("kB")) {
          String[] strings = line.split("\\s+");
          if (strings.length > 1) {
            usage.vssKbSize = Long.parseLong(strings[1]);
          }
        } else if (line.startsWith("VmRSS:") && line.contains("kB")) {
          String[] strings = line.split("\\s+");
          if (strings.length > 1) {
            usage.rssKbSize = Long.parseLong(strings[1]);
          }
        } else if (line.startsWith("Threads:")) {
          String[] strings = line.split("\\s+");
          if (strings.length > 1) {
            usage.threadsCount = Integer.parseInt(strings[1]);
          }
        }
      }
    } catch (IOException ex) {
      ex.printStackTrace();
    } finally {
      closeQuietly(reader);
    }
    return usage;
  }

  public static void closeQuietly(Closeable closeable) {
    try {
      if (closeable != null) {
        closeable.close();
      }
    } catch (IOException ioe) {
      // ignore
    }
  }
}
