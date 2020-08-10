package com.kwai.koom.javaoom.monitor;

import com.kwai.koom.javaoom.common.KConstants;

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
public class HeapThrashingThreshold implements Threshold {

  private static final int DEFAULT_THRASH_SIZE = 100;
  private static final int DEFAULT_OVER_TIMES = 3;
  private static final int DEFAULT_POLL_INTERVAL = 5000;

  @Override
  public float value() {
    return DEFAULT_THRASH_SIZE * KConstants.Bytes.MB;
  }

  @Override
  public int overTimes() {
    return DEFAULT_OVER_TIMES;
  }

  @Override
  public boolean ascending() {
    return false;
  }

  @Override
  public ThresholdValueType valueType() {
    return ThresholdValueType.BYTES;
  }

  @Override
  public int pollInterval() {
    return DEFAULT_POLL_INTERVAL;
  }
}
