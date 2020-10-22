package com.kwai.koom.javaoom.common;

import static com.kwai.koom.javaoom.common.KConstants.Bytes.MB;

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
public class KConstants {

  public static class Bytes {
    public static int KB = 1024;
    public static int MB = 1024 * KB;
    public static int GB = 1024 * MB;
  }

  public static class HeapThreshold {
    public static int VM_512_DEVICE = 510;
    public static int VM_256_DEVICE = 250;
    public static int VM_128_DEVICE = 128;

    public static float PERCENT_RATIO_IN_512_DEVICE = 80;
    public static float PERCENT_RATIO_IN_256_DEVICE = 85;
    public static float PERCENT_RATIO_IN_128_DEVICE = 90;

    public static float PERCENT_MAX_RATIO = 95;

    public static float getDefaultPercentRation() {
      int maxMem = (int) (Runtime.getRuntime().maxMemory() / MB);
      if (Debug.VERBOSE_LOG) {
        KLog.i("koom", "max mem " + maxMem);
      }
      if (maxMem >= VM_512_DEVICE) {
        return KConstants.HeapThreshold.PERCENT_RATIO_IN_512_DEVICE;
      } else if (maxMem >= VM_256_DEVICE) {
        return KConstants.HeapThreshold.PERCENT_RATIO_IN_256_DEVICE;
      } else if (maxMem >= VM_128_DEVICE) {
        return KConstants.HeapThreshold.PERCENT_RATIO_IN_128_DEVICE;
      }
      return KConstants.HeapThreshold.PERCENT_RATIO_IN_512_DEVICE;
    }

    public static float getDefaultMaxPercentRation() {
      return KConstants.HeapThreshold.PERCENT_MAX_RATIO;
    }

    public static int OVER_TIMES = 3;
    public static int POLL_INTERVAL = 5000;
  }

  public static class ArrayThreshold {
    public static final int DEFAULT_BIG_PRIMITIVE_ARRAY = 256 * 1024;//基本数组大小阈值
    public static final int DEFAULT_BIG_OBJECT_ARRAY = 256 * 1024;//对象数组大小阈值
  }

  public static class BitmapThreshold {
    public static final int DEFAULT_BIG_WIDTH = 768;
    public static final int DEFAULT_BIG_HEIGHT = 1366;
    public static final int DEFAULT_BIG_BITMAP = DEFAULT_BIG_WIDTH * DEFAULT_BIG_HEIGHT;//大bitmap阈值
  }

  public static class ServiceIntent {
    public static final String RECEIVER = "receiver";
    public static final String HEAP_FILE = "heap_file";
  }

  public static class Perf {
    public static final int START_DELAY = 10000;
  }

  public static class KOOMVersion {
    public static int CODE = 1;
    public static String NAME = "1.0";
  }

  public static class Debug {
    public static boolean VERBOSE_LOG = true;
  }

  public static class ReAnalysis {
    public static int MAX_TIMES = 2;
  }

  public static class Disk {
    public static float ENOUGH_SPACE_IN_GB = 5;
  }

  public static class SP {
    public static String TRIGGER_TIMES_NAME = "_koom_trigger_times";
    public static String FIRST_LAUNCH_TIME_NAME = "_koom_first_launch_time";
  }

  public static class EnableCheck {
    public static int TRIGGER_MAX_TIMES = 3;
    public static int MAX_TIME_WINDOW_IN_DAYS = 15;
  }

  public static class Time {
    public static long DAY_IN_MILLS = 1000 * 24 * 60 * 60;
  }
}
