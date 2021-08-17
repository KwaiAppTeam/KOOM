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
 * Created by lbtrace on 2021.
 *
 */

package com.kwai.koom.nativeoom.leakmonitor.allocationtag

import com.kwai.koom.nativeoom.leakmonitor.LeakMonitor

data class AllocationTagInfo(
    val tag: String
) {
  var allocationStartIndex = 0L
  var startTime = -1L

  var allocationEndIndex = 0L
  var endTime = -1L
}

internal fun AllocationTagInfo.end() {
  this.allocationEndIndex = LeakMonitor.getAllocationIndex()
  this.endTime = System.currentTimeMillis()
}

internal fun String.createAllocationTagInfo(): AllocationTagInfo {
  return AllocationTagInfo(this).apply {
    this.allocationStartIndex = LeakMonitor.getAllocationIndex()
    this.startTime = System.currentTimeMillis()

    this.allocationEndIndex = -1L
    this.endTime = -1L
  }
}

internal fun AllocationTagInfo.searchTag(allocationIndex: Long): String? {
  if (allocationIndex < allocationStartIndex) {
    return null
  }

  return if (contains(allocationIndex)) tag else null
}

private fun AllocationTagInfo.contains(allocationIndex: Long): Boolean {
  if (allocationIndex < allocationStartIndex) {
    return false
  }

  return if (allocationEndIndex == -1L) {
    true
  } else {
    allocationIndex <= allocationEndIndex
  }
}