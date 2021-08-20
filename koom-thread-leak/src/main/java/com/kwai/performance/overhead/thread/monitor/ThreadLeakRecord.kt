/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by shenvsv on 2021.
 *
 */

package com.kwai.performance.overhead.thread.monitor

import androidx.annotation.Keep

@Keep
data class ThreadLeakRecord(
    val tid: Int,
    val createTime: Long,
    val startTime: Long,
    val endTime: Long,
    val name: String,
    val createCallStack: String) {

  override fun toString(): String = StringBuilder().apply {
    append("tid: $tid\n")
    append("createTime: $createTime Byte\n")
    append("startTime: $startTime\n")
    append("endTime: $endTime\n")
    append("name: $name\n")
    append("createCallStack:\n")
    append(createCallStack)
  }.toString()
}

@Keep
data class ThreadLeakContainer(
    val type: String,
    val threads: MutableList<ThreadLeakRecord>)