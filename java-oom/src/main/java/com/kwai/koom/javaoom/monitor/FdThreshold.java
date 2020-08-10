package com.kwai.koom.javaoom.monitor;

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
public class FdThreshold implements Threshold {

  private static final int DEFAULT_POLL_INTERVAL = 15000;
  private static final int DEFAULT_OVER_TIMES = 3;
  private static final int DEFAULT_FD_COUNT = 800;

  @Override
  public float value() {
    return DEFAULT_FD_COUNT;
  }

  @Override
  public int overTimes() {
    return DEFAULT_OVER_TIMES;
  }

  @Override
  public ThresholdValueType valueType() {
    return ThresholdValueType.COUNT;
  }

  @Override
  public boolean ascending() {
    return true;
  }

  @Override
  public int pollInterval() {
    return DEFAULT_POLL_INTERVAL;
  }
}
