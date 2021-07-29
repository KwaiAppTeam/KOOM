package com.kwai.koom.demo.nativeleak.init

import android.app.Application

interface InitTask {
  fun init(application: Application)
}