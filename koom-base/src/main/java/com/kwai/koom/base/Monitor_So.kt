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
 * @author KOOM Team
 *
 */
package com.kwai.koom.base

import android.os.Build
import android.util.Log

private const val TAG = "MonitorSo"

@Deprecated("Deprecated", ReplaceWith("loadSoQuietly(soName)"))
fun loadSo(soName: String) = MonitorManager.commonConfig.loadSoInvoker(soName)

fun loadSoQuietly(soName: String): Boolean = runCatching {
  MonitorManager.commonConfig.loadSoInvoker(soName)

  return@runCatching true
}.onFailure {
  it.printStackTrace()

  MonitorLog.e(TAG, it.message + "\n"+ Log.getStackTraceString(it))
}.getOrElse { false }

fun isSupportArm64(): Boolean {
  if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
    return false
  }

  return supportedABIs().contains("arm64-v8a")
}

private fun supportedABIs(): Array<String> {
  return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
      && Build.SUPPORTED_ABIS.isNotEmpty()) {
    Build.SUPPORTED_ABIS
  } else if (!Build.CPU_ABI2.isNullOrEmpty()) {
    arrayOf(Build.CPU_ABI, Build.CPU_ABI2)
  } else {
    arrayOf(Build.CPU_ABI)
  }
}
