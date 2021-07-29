package com.kwai.koom.base

import android.app.Application
import android.content.Context
import android.content.SharedPreferences
import android.os.Handler
import com.kwai.koom.base.loop.LoopThread
import java.io.File
import java.util.concurrent.ExecutorService

class CommonConfig private constructor(
    // MonitorManager 通用属性
    val application: Application,

    // 子Monitor模块定制类xxxFileManager、xxPreferencesManager
    val rootFileInvoker: (String) -> File,
    val sharedPreferencesInvoker: (String) -> SharedPreferences,
    val sharedPreferencesKeysInvoker: (SharedPreferences) -> Set<String>,

    // MonitorBuildConfig 通用属性
    internal val debugMode: Boolean,
    internal val productNameInvoker: () -> String,
    internal val versionNameInvoker: () -> String,
    internal val serviceIdInvoker: () -> String,
    internal val channelInvoker: () -> String,
    internal val deviceIdInvoker: () -> String,
    internal val romInvoker: () -> String,

    internal val logger: Logger,
    internal val log: Log,

    // 工具类
    internal val loadSoInvoker: (String) -> Unit,
    internal val executorServiceInvoker: (() -> ExecutorService)?,

    // LoopMonitor 使用
    internal val loopHandlerInvoker: () -> Handler
) {
  class Builder {
    private lateinit var mApplication: Application

    private var mDebugMode = true
    private lateinit var mProductNameInvoker: () -> String
    private lateinit var mVersionNameInvoker: () -> String
    private lateinit var mServiceIdInvoker: (() -> String)
    private lateinit var mChannelInvoker: (() -> String)
    private lateinit var mDeviceIdInvoker: (() -> String)
    private lateinit var mRomInvoker: (() -> String)

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

    fun setProductNameInvoker(productNameInvoker: () -> String) = apply {
      mProductNameInvoker = productNameInvoker
    }

    fun setVersionNameInvoker(versionNameInvoker: () -> String) = apply {
      mVersionNameInvoker = versionNameInvoker
    }

    fun setChannelInvoker(channelInvoker: () -> String) = apply {
      mChannelInvoker = channelInvoker
    }

    fun setServiceIdInvoker(serviceIdInvoker: () -> String) = apply {
      mServiceIdInvoker = serviceIdInvoker
    }

    fun setDeviceIdInvoker(deviceIdInvoker: () -> String) = apply {
      mDeviceIdInvoker = deviceIdInvoker
    }

    fun setRomInvoker(romInvoker: () -> String) = apply {
      mRomInvoker = romInvoker
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
        productNameInvoker = mProductNameInvoker,
        versionNameInvoker = mVersionNameInvoker,
        serviceIdInvoker = mServiceIdInvoker,
        channelInvoker = mChannelInvoker,
        deviceIdInvoker = mDeviceIdInvoker,
        romInvoker = mRomInvoker,

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