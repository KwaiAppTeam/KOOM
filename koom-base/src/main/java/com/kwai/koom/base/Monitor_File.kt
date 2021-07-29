package com.kwai.koom.base

import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

/**
 * 压缩时只保留最后一级文件名，不保留路径，不同路径下的重名文件会被覆盖
 */
const val ZIP_LAST_PATH_NAME = -1

/**
 * 压缩时保留原始目录结构
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