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