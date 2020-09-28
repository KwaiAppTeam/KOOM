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
public class HeapThreshold implements Threshold {

    private float heapRatioInPercent;
    private float heapMaxRatioInPercent;
    private int overTimes;
    private int pollInterval;

    public HeapThreshold(float heapRatioInPercent, float heapMaxRatioInPercent, int overTimes, int pollInterval) {
        this.heapRatioInPercent = heapRatioInPercent;
        this.heapMaxRatioInPercent =heapMaxRatioInPercent;
        this.overTimes = overTimes;
        this.pollInterval = pollInterval;
    }

    @Override
    public float value() {
        return heapRatioInPercent;
    }

  @Override
  public float maxValue() {
    return heapMaxRatioInPercent;
  }

  @Override
    public int overTimes() {
        return overTimes;
    }

    @Override
    final public ThresholdValueType valueType() {
        return ThresholdValueType.PERCENT;
    }

    @Override
    public boolean ascending() {
        return true;
    }

    @Override
    public int pollInterval() {
        return pollInterval;
    }
}
