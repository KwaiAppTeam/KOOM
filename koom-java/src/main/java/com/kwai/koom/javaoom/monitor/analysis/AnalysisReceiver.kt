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
 * <p>
 * KOOM library entry point.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.monitor.analysis

import android.os.Bundle
import android.os.ResultReceiver

class AnalysisReceiver : ResultReceiver(null) {
  private var mResultCallBack: ResultCallBack? = null

  fun setResultCallBack(resultCallBack: ResultCallBack?) {
    mResultCallBack = resultCallBack
  }

  override fun onReceiveResult(resultCode: Int, resultData: Bundle?) {
    super.onReceiveResult(resultCode, resultData)
    if (mResultCallBack != null) {
      if (resultCode == RESULT_CODE_OK) {
        mResultCallBack!!.onSuccess()
      } else {
        mResultCallBack!!.onError()
      }
    }
  }

  interface ResultCallBack {
    fun onSuccess()
    fun onError()
  }

  companion object {
    const val RESULT_CODE_OK = 1001
    const val RESULT_CODE_FAIL = 1002
  }
}