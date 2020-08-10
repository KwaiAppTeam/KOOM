package com.kwai.koom.javaoom.analysis;

import android.os.Bundle;
import android.os.ResultReceiver;

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
 * ResultReceiver of IntentService for callback of inter process communication.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */
class IPCReceiver extends ResultReceiver {

  public static final int RESULT_CODE_OK = 1001;
  public static final int RESULT_CODE_FAIL = 1002;

  private ReceiverCallback receiverCallBack;

  public IPCReceiver(ReceiverCallback receiverCallBack) {
    //null means call back from an arbitrary thread
    super(null);

    this.receiverCallBack = receiverCallBack;
  }

  @Override
  protected void onReceiveResult(int resultCode, Bundle resultData) {
    super.onReceiveResult(resultCode, resultData);
    if (receiverCallBack != null) {
      if (resultCode == RESULT_CODE_OK) {
        receiverCallBack.onSuccess();
      } else {
        receiverCallBack.onError();
      }
    }
  }

  public interface ReceiverCallback {
    void onSuccess();

    void onError();
  }
}
