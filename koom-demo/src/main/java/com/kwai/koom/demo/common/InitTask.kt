package com.kwai.koom.demo.common

import android.app.Application

interface InitTask {
  fun init(application: Application)
}