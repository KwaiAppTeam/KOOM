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

package com.kwai.koom.javaoom.monitor.tracker

import com.kwai.koom.base.Monitor
import com.kwai.koom.javaoom.monitor.OOMMonitorConfig

abstract class OOMTracker : Monitor<OOMMonitorConfig>() {
  /**
   * @return true 表示追踪到oom、 false 表示没有追踪到oom
   */
  abstract fun track(): Boolean

  /**
   * 重置track状态
   */
  abstract fun reset()

  /**
   * @return 追踪到的oom的标识
   */
  abstract fun reason(): String
}