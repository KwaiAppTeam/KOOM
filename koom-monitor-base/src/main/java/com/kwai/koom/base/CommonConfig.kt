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

import android.app.Application
import android.content.Context
import android.content.SharedPreferences
import android.os.Handler
import com.kwai.koom.base.loop.LoopThread
import java.io.File
import java.util.concurrent.ExecutorService

class CommonConfig private constructor(
    // MonitorManager common properties
    val application: Application,

    // Custom FileManager or sharedPreferences
    val rootFileInvoker: (String) -> File,
    val sharedPreferencesInvoker: (String) -> SharedPreferences,
    val sharedPreferencesKeysInvoker: (SharedPreferences) -> Set<String>,

    // MonitorBuildConfig common properties
    internal val debugMode: Boolean,
    internal val sdkVersionMatch: Boolean,
    internal val versionNameInvoker: () -> String,

    internal val logger: Logger,
    internal val log: Log,

    // toolbox
    internal val loadSoInvoker: (String) -> Unit,
    internal val executorServiceInvoker: (() -> ExecutorService)?,

    // For LooperMonitor
    internal val loopHandlerInvoker: () -> Handler
) {
  class Builder {
    private lateinit var mApplication: Application

    private var mDebugMode = true
    private var mSdkVersionMatch = false
    private lateinit var mVersionNameInvoker: () -> String
    private lateinit var mDeviceIdInvoker: (() -> String)

    private var mRootFileInvoker: ((String) -> File)? = null
    private var mSharedPreferencesInvoker: ((String) -> SharedPreferences)? = null
    private var mSharedPreferencesKeysInvoker: ((SharedPreferences) -> Set<String>)? = null

    private var mLogger: Logger? = null
    private var mLog: Log? = null

    private var mLoadSoInvoker: ((String) -> Unit)? = null
    private var mExecutorServiceInvoker: (() -> ExecutorService)? = null

    private var mLoopHandlerInvoker: (() -> Handler)? = null

    fun setApplication(application: Application) = apply {
      mApplication = application
    }

    fun setDebugMode(debugMode: Boolean) = apply {
      mDebugMode = debugMode
    }

    fun setSdkVersionMatch(sdkVersionMatch: Boolean) = apply {
      mSdkVersionMatch = sdkVersionMatch
    }

    fun setVersionNameInvoker(versionNameInvoker: () -> String) = apply {
      mVersionNameInvoker = versionNameInvoker
    }

    fun setRootFileInvoker(rootFileInvoker: (String) -> File) = apply {
      mRootFileInvoker = rootFileInvoker
    }

    fun setSharedPreferencesInvoker(sharedPreferencesInvoker: (String) -> SharedPreferences) = apply {
      mSharedPreferencesInvoker = sharedPreferencesInvoker
    }

    fun setSharedPreferencesKeysInvoker(
        sharedPreferencesKeysInvoker: (SharedPreferences) -> Set<String>
    ) = apply {
      mSharedPreferencesKeysInvoker = sharedPreferencesKeysInvoker
    }

    fun setLoadSoInvoker(LoadSoInvoker: (String) -> Unit) = apply {
      mLoadSoInvoker = LoadSoInvoker
    }

    fun setLogger(logger: Logger) = apply {
      mLogger = logger
    }

    fun setLog(log: Log) = apply {
      mLog = log
    }

    fun setExecutorServiceInvoker(executorServiceInvoker: () -> ExecutorService) = apply {
      mExecutorServiceInvoker = executorServiceInvoker
    }

    fun setLoopHandlerInvoker(loopHandlerInvoker: () -> Handler) = apply {
      mLoopHandlerInvoker = loopHandlerInvoker
    }

    fun build(): CommonConfig = CommonConfig(
        application = mApplication,

        debugMode = mDebugMode,
        sdkVersionMatch = mSdkVersionMatch,
        versionNameInvoker = mVersionNameInvoker,

        rootFileInvoker = mRootFileInvoker ?: {
          val rootDir = runCatching { mApplication.getExternalFilesDir("") }.getOrNull()

          File(rootDir ?: mApplication.filesDir, "performance/$it")
              .apply { mkdirs() }
        },

        sharedPreferencesInvoker = mSharedPreferencesInvoker ?: {
          mApplication.getSharedPreferences("performance", Context.MODE_PRIVATE)
        },
        sharedPreferencesKeysInvoker = mSharedPreferencesKeysInvoker ?: { it.all.keys },

        logger = mLogger ?: object : Logger {},
        log = mLog ?: object : Log {},

        loadSoInvoker = mLoadSoInvoker ?: { System.loadLibrary(it) },
        executorServiceInvoker = mExecutorServiceInvoker,

        loopHandlerInvoker = mLoopHandlerInvoker ?: { LoopThread.LOOP_HANDLER }
    )
  }
}