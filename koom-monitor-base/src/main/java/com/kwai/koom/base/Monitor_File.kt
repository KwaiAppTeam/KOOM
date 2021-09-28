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
import java.io.FileInputStream
import java.io.FileOutputStream
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

/**
 * When compressing, only the last-level file name is retained, and the path is not retained.
 * Files with the same name under different paths will be overwritten
 */
const val ZIP_LAST_PATH_NAME = -1

/**
 * Keep original directory structure when compressing
 */
const val ZIP_FULL_PATH_NAME = 0

fun File.zipTo(zipFile: File, zipType: Int = ZIP_LAST_PATH_NAME) {
  if (isFile) {
    arrayListOf(this).zipTo(zipFile.absolutePath, zipType)
  } else if (isDirectory) {
    arrayListOf<File>().apply { buildSrcFileList(this) }
        .zipTo(zipFile.absolutePath, zipType)
  }
}

fun List<File>.zipTo(zipFilePath: String, zipType: Int = ZIP_LAST_PATH_NAME) {
  ZipOutputStream(FileOutputStream(zipFilePath)).use { out ->
    for (file in this) {
      val filePath = file.absolutePath

      if (zipType == ZIP_LAST_PATH_NAME) {
        ZipEntry(filePath.substring(filePath.lastIndexOf("/") + 1))
      } else {
        ZipEntry(filePath)
      }.also {
        out.putNextEntry(it)
      }

      FileInputStream(file).use { it.copyTo(out) }
    }
  }
}

fun File.readFirstLine(): String? {
  useLines { return it.firstOrNull() }
}

private fun File.buildSrcFileList(srcFileList: MutableList<File>) {
  for (file in listFiles().orEmpty()) {
    if (file.isDirectory) {
      file.buildSrcFileList(srcFileList)
    } else if (file.isFile) {
      srcFileList.add(file)
    }
  }
}