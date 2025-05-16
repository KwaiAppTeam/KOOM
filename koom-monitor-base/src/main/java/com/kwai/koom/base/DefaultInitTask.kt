/*
 * Copyright (c) 2022. Kwai, Inc. All rights reserved.
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
import android.os.Build

object DefaultInitTask : InitTask {
  override fun init(application: Application) {
    val config = CommonConfig.Builder()
      .setApplication(application) // Set application
      .setVersionNameInvoker { "1.0.0" } // Set version name, java leak feature use it
      .setSdkVersionMatch(
        Build.VERSION.SDK_INT <= 36 &&
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
      )  // Set if current sdk version is supported
      .build()

    MonitorManager.initCommonConfig(config)
      .apply { onApplicationCreate() }
  }
}