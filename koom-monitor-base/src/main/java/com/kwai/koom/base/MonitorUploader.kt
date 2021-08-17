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

import java.io.File

interface MonitorUploader<T> {
  fun uploadFile(params: MutableMap<String, Any>, file: File): T

  companion object {
    const val FIELD_BIZ_TYPE = "bizType"
    const val FIELD_FILE_EXTEND = "fileExtend"
    const val FIELD_EXTRA_INFO = "extraInfo"
    const val FIELD_UPLOAD_TOKEN = "uploadToken"
    const val FIELD_SID = "sid"
    const val FIELD_DID = "did"
  }
}