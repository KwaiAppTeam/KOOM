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

import com.kwai.koom.base.MonitorLog
import com.kwai.koom.javaoom.hprof.ForkJvmHeapDumper
import com.kwai.koom.javaoom.hprof.ForkStripHeapDumper
import com.kwai.koom.javaoom.hprof.HeapDumper
import com.kwai.koom.javaoom.hprof.StandardHeapDumper
import com.kwai.koom.javaoom.hprof.StripHprofHeapDumper
import com.kwai.koom.javaoom.monitor.utils.SizeUnit
import java.util.*

object OOMHeapDumper {
  private const val TAG = "OOMHeapDumper"

  private fun dump(dumper: HeapDumper) {
    try {
      MonitorLog.i(TAG, "dump hprof start")

      val hprofFile = OOMFileManager.createHprofOOMDumpFile(Date())

      val start = System.currentTimeMillis()

      hprofFile.createNewFile()
      dumper.dump(hprofFile.absolutePath)

      val end = System.currentTimeMillis()

      MonitorLog.i(TAG, "dump hprof complete," +
          " dumpTime:" + (end - start) +
          " fileName:" + hprofFile.name +
          " origin fileSize:" + SizeUnit.BYTE.toMB(hprofFile.length()) +
          " JVM max memory:" + SizeUnit.BYTE.toMB(Runtime.getRuntime().maxMemory()) +
          " JVM  free memory:" + SizeUnit.BYTE.toMB(Runtime.getRuntime().freeMemory()) +
          " JVM total memory:" + SizeUnit.BYTE.toMB(Runtime.getRuntime().totalMemory()), true)
    } catch (e: Throwable) {
      e.printStackTrace()

      MonitorLog.i(TAG, "dumpStripHprof failed: ${e.message}")
    }
  }

  @JvmStatic
  fun simpleDump() {
    MonitorLog.i(TAG, "simpleDump")
    dump(StandardHeapDumper())
  }

  @JvmStatic
  fun forkDump() {
    MonitorLog.i(TAG, "forkDump")
    dump(ForkJvmHeapDumper())
  }

  @JvmStatic
  fun stripDump() {
    MonitorLog.i(TAG, "dumpStripHprof")
    dump(StripHprofHeapDumper())
  }

  @JvmStatic
  fun forkDumpStrip() {
    MonitorLog.i(TAG, "forkDumpStrip")

    dump(ForkStripHeapDumper())
  }

}