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

package com.kwai.koom.javaoom.monitor

import android.os.StatFs
import com.kwai.koom.base.MonitorBuildConfig
import java.io.File
import java.text.SimpleDateFormat
import java.util.*

internal object OOMFileManager {
  private const val TIME_FORMAT = "yyyy-MM-dd_HH-mm-ss_SSS"

  private lateinit var mRootDirInvoker: (String) -> File
  private lateinit var mPrefix: String

  private lateinit var mRootPath: String

  val rootDir by lazy {
    if (this::mRootDirInvoker.isInitialized)
      mRootDirInvoker("oom")
    else
      File(mRootPath)
  }

  @JvmStatic
  val hprofAnalysisDir by lazy { File(rootDir, "memory/hprof-aly").apply { mkdirs() } }

  @JvmStatic
  val manualDumpDir by lazy { File(rootDir, "memory/hprof-man").apply { mkdirs() } }

  @JvmStatic
  val threadDumpDir by lazy { File(hprofAnalysisDir, "thread").apply { mkdirs() } }

  @JvmStatic
  val fdDumpDir by lazy { File(hprofAnalysisDir, "fd").apply { mkdirs() } }

  @JvmStatic
  fun init(rootDirInvoker: (String) -> File) {
    mRootDirInvoker = rootDirInvoker
    mPrefix = "${MonitorBuildConfig.VERSION_NAME}_"
  }

  @JvmStatic
  fun init(rootPath: String?) {
    if (rootPath != null) {
      mRootPath = rootPath
    }
    mPrefix = "${MonitorBuildConfig.VERSION_NAME}_"
  }

  @JvmStatic
  fun createHprofAnalysisFile(date: Date): File {
    val time = SimpleDateFormat(TIME_FORMAT, Locale.CHINESE).format(date)
    return File(hprofAnalysisDir, "$mPrefix$time.hprof").also {
      hprofAnalysisDir.mkdirs()
    }
  }

  @JvmStatic
  fun createJsonAnalysisFile(date: Date): File {
    val time = SimpleDateFormat(TIME_FORMAT, Locale.CHINESE).format(date)
    return File(hprofAnalysisDir, "$mPrefix$time.json").also {
      hprofAnalysisDir.mkdirs()
    }
  }

  @JvmStatic
  fun createHprofOOMDumpFile(date: Date): File {
    val time = SimpleDateFormat(TIME_FORMAT, Locale.CHINESE).format(date)
    return File(manualDumpDir, "$mPrefix$time.hprof").also {
      manualDumpDir.mkdirs()
    }
  }

  @JvmStatic
  fun createDumpFile(dumpDir: File): File {
    return File(dumpDir, "dump.txt").also {
      dumpDir.mkdirs()
    }
  }

  @JvmStatic
  fun isSpaceEnough(): Boolean {
    val statFs = StatFs(hprofAnalysisDir.canonicalPath)
    val blockSize = statFs.blockSizeLong
    val availableBlocks = statFs.availableBlocks.toLong()

    return blockSize * availableBlocks > 1.2 * 1024 * 1024
  }
}