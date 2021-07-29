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
