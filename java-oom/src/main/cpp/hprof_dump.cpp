// Copyright 2020 Kwai, Inc. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//         http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//         limitations under the License.
//
// Created by lirui on 2019-12-17.
//

#include <android/log.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <jni.h>
#include <kwai_dlfcn.h>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <wait.h>
#include <xhook.h>

#define LOG_TAG "HprofDump"

enum HprofTag {
  HPROF_TAG_STRING = 0x01,
  HPROF_TAG_LOAD_CLASS = 0x02,
  HPROF_TAG_UNLOAD_CLASS = 0x03,
  HPROF_TAG_STACK_FRAME = 0x04,
  HPROF_TAG_STACK_TRACE = 0x05,
  HPROF_TAG_ALLOC_SITES = 0x06,
  HPROF_TAG_HEAP_SUMMARY = 0x07,
  HPROF_TAG_START_THREAD = 0x0A,
  HPROF_TAG_END_THREAD = 0x0B,
  HPROF_TAG_HEAP_DUMP = 0x0C,
  HPROF_TAG_HEAP_DUMP_SEGMENT = 0x1C,
  HPROF_TAG_HEAP_DUMP_END = 0x2C,
  HPROF_TAG_CPU_SAMPLES = 0x0D,
  HPROF_TAG_CONTROL_SETTINGS = 0x0E,
};

enum HprofHeapTag {
  // Traditional.
  HPROF_ROOT_UNKNOWN = 0xFF,
  HPROF_ROOT_JNI_GLOBAL = 0x01,
  HPROF_ROOT_JNI_LOCAL = 0x02,
  HPROF_ROOT_JAVA_FRAME = 0x03,
  HPROF_ROOT_NATIVE_STACK = 0x04,
  HPROF_ROOT_STICKY_CLASS = 0x05,
  HPROF_ROOT_THREAD_BLOCK = 0x06,
  HPROF_ROOT_MONITOR_USED = 0x07,
  HPROF_ROOT_THREAD_OBJECT = 0x08,
  HPROF_CLASS_DUMP = 0x20,
  HPROF_INSTANCE_DUMP = 0x21,
  HPROF_OBJECT_ARRAY_DUMP = 0x22,
  HPROF_PRIMITIVE_ARRAY_DUMP = 0x23,

  // Android.
  HPROF_HEAP_DUMP_INFO = 0xfe,
  HPROF_ROOT_INTERNED_STRING = 0x89,
  HPROF_ROOT_FINALIZING = 0x8a, // Obsolete.
  HPROF_ROOT_DEBUGGER = 0x8b,
  HPROF_ROOT_REFERENCE_CLEANUP = 0x8c, // Obsolete.
  HPROF_ROOT_VM_INTERNAL = 0x8d,
  HPROF_ROOT_JNI_MONITOR = 0x8e,
  HPROF_UNREACHABLE = 0x90,                 // Obsolete.
  HPROF_PRIMITIVE_ARRAY_NODATA_DUMP = 0xc3, // Obsolete.
};

enum HprofBasicType {
  hprof_basic_object = 2,
  hprof_basic_boolean = 4,
  hprof_basic_char = 5,
  hprof_basic_float = 6,
  hprof_basic_double = 7,
  hprof_basic_byte = 8,
  hprof_basic_short = 9,
  hprof_basic_int = 10,
  hprof_basic_long = 11,
};

enum HprofHeapId {
  HPROF_HEAP_DEFAULT = 0,
  HPROF_HEAP_ZYGOTE = 'Z',
  HPROF_HEAP_APP = 'A',
  HPROF_HEAP_IMAGE = 'I',
};

enum HprofTagBytes {
  OBJECT_ID_BYTE_SIZE = 4,
  JNI_GLOBAL_REF_ID_BYTE_SIZE = 4,
  CLASS_ID_BYTE_SIZE = 4,
  CLASS_LOADER_ID_BYTE_SIZE = 4,
  INSTANCE_SIZE_BYTE_SIZE = 4,
  CONSTANT_POOL_LENGTH_BYTE_SIZE = 2,
  STATIC_FIELD_LENGTH_BYTE_SIZE = 2,
  INSTANCE_FIELD_LENGTH_BYTE_SIZE = 2,
  STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE = 4,
  RECORD_TIME_BYTE_SIZE = 4,
  RECORD_LENGTH_BYTE_SIZE = 4,
  STRING_ID_BYTE_SIZE = 4,

  HEAP_TAG_BYTE_SIZE = 1,
  THREAD_SERIAL_BYTE_SIZE = 4,
  CONSTANT_POLL_INDEX_BYTE_SIZE = 2,
  BASIC_TYPE_BYTE_SIZE = 1,
  HEAP_TYPE_BYTE_SIZE = 4,
};

// const int U1 = 1;
const int U4 = 4;
int hprofFd = -1;
char *hprofName = nullptr;
bool isDumpHookSucc = false;

int hook_open(const char *pathname, int flags, ...) {
  va_list ap;
  va_start(ap, flags);
  int fd = open(pathname, flags, ap);
  va_end(ap);

  if (hprofName == nullptr) {
    return fd;
  }

  if (pathname != nullptr && strstr(pathname, hprofName)) {
    hprofFd = fd;
    isDumpHookSucc = true;
  }
  return fd;
}

int getShortFromBytes(const unsigned char *buf, int index) {
  return (buf[index] << 8u) + buf[index + 1];
}

int getIntFromBytes(const unsigned char *buf, int index) {
  return (buf[index] << 24u) + (buf[index + 1] << 16u) + (buf[index + 2] << 8u) + buf[index + 3];
}

int getByteSizeFromType(unsigned char basicType) {
  switch (basicType) {
  case hprof_basic_boolean:
  case hprof_basic_byte:
    return 1;
  case hprof_basic_char:
  case hprof_basic_short:
    return 2;
  case hprof_basic_float:
  case hprof_basic_int:
  case hprof_basic_object:
    return 4;
  case hprof_basic_long:
  case hprof_basic_double:
    return 8;
  default:
    return 0;
  }
}

int stripBytesSum = 0;
int heapSerialNum = 0;
int hookWriteSerialNum = 0;

const int STRIP_LIST_LENGTH = 65536 * 2 * 2 + 2;
int stripIndexListPair[STRIP_LIST_LENGTH];
int stripIndex = 0;

int isCurrentSystemHeap = false;

int processHeap(const void *buf, int firstIndex, int maxLen, int heapSerialNo, int arraySerialNo) {
  if (firstIndex >= maxLen) {
    return arraySerialNo;
  }

  const unsigned char subtag = ((unsigned char *)buf)[firstIndex];
  switch (subtag) {
  /**
   * __ AddU1(heap_tag);
   * __ AddObjectId(obj);
   *
   */
  case HPROF_ROOT_UNKNOWN:
  case HPROF_ROOT_STICKY_CLASS:
  case HPROF_ROOT_MONITOR_USED:
  case HPROF_ROOT_INTERNED_STRING:
  case HPROF_ROOT_DEBUGGER:
  case HPROF_ROOT_VM_INTERNAL: {
    arraySerialNo = processHeap(buf, firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE, maxLen,
                                heapSerialNo, arraySerialNo);
  } break;

  case HPROF_ROOT_JNI_GLOBAL: {
    /**
     *  __ AddU1(heap_tag);
     *  __ AddObjectId(obj);
     *  __ AddJniGlobalRefId(jni_obj);
     *
     */
    arraySerialNo = processHeap(
        buf, firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE + JNI_GLOBAL_REF_ID_BYTE_SIZE,
        maxLen, heapSerialNo, arraySerialNo);
  } break;

    /**
     * __ AddU1(heap_tag);
     * __ AddObjectId(obj);
     * __ AddU4(thread_serial);
     * __ AddU4((uint32_t)-1);
     */
  case HPROF_ROOT_JNI_LOCAL:
  case HPROF_ROOT_JAVA_FRAME:
  case HPROF_ROOT_JNI_MONITOR: {
    arraySerialNo = processHeap(buf,
                                firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE +
                                    THREAD_SERIAL_BYTE_SIZE + U4 /*占位*/,
                                maxLen, heapSerialNo, arraySerialNo);
  } break;

    /**
     * __ AddU1(heap_tag);
     * __ AddObjectId(obj);
     * __ AddU4(thread_serial);
     */
  case HPROF_ROOT_NATIVE_STACK:
  case HPROF_ROOT_THREAD_BLOCK: {
    arraySerialNo = processHeap(
        buf, firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE + THREAD_SERIAL_BYTE_SIZE,
        maxLen, heapSerialNo, arraySerialNo);
  } break;

    /**
     * __ AddU1(heap_tag);
     * __ AddObjectId(obj);
     * __ AddU4(thread_serial);
     * __ AddU4((uint32_t)-1);    // xxx
     */
  case HPROF_ROOT_THREAD_OBJECT: {
    arraySerialNo = processHeap(buf,
                                firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE +
                                    THREAD_SERIAL_BYTE_SIZE + U4 /*占位*/,
                                maxLen, heapSerialNo, arraySerialNo);
  } break;

    /**
     * __ AddU1(HPROF_CLASS_DUMP);
     * __ AddClassId(LookupClassId(klass));
     * __ AddStackTraceSerialNumber(LookupStackTraceSerialNumber(klass));
     * __ AddClassId(LookupClassId(klass->GetSuperClass().Ptr()));
     * __ AddObjectId(klass->GetClassLoader().Ptr());
     * __ AddObjectId(nullptr);    // no signer
     * __ AddObjectId(nullptr);    // no prot domain
     * __ AddObjectId(nullptr);    // reserved
     * __ AddObjectId(nullptr);    // reserved
     * __ AddU4(0); 或 __ AddU4(sizeof(mirror::String)); 或 __ AddU4(0); 或 __
     * AddU4(klass->GetObjectSize());  // instance size
     * __ AddU2(0);  // empty const pool
     * __ AddU2(dchecked_integral_cast<uint16_t>(static_fields_reported));
     * static_field_writer(class_static_field, class_static_field_name_fn);
     */
  case HPROF_CLASS_DUMP: {

    /**
     *  u2
        size of constant pool and number of records that follow:
            u2
            constant pool index
            u1
            type of entry: (See Basic Type)
            value
            value of entry (u1, u2, u4, or u8 based on type of entry)
     */
    int constantPoolIndex = firstIndex + HEAP_TAG_BYTE_SIZE /*tag*/
                            + CLASS_ID_BYTE_SIZE + STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE +
                            CLASS_ID_BYTE_SIZE /*super*/ + CLASS_LOADER_ID_BYTE_SIZE +
                            OBJECT_ID_BYTE_SIZE   // Ignored: Signeres ID.
                            + OBJECT_ID_BYTE_SIZE // Ignored: Protection domain ID.
                            + OBJECT_ID_BYTE_SIZE // RESERVED.
                            + OBJECT_ID_BYTE_SIZE // RESERVED.
                            + INSTANCE_SIZE_BYTE_SIZE;
    int constantPoolSize = getShortFromBytes((unsigned char *)buf, constantPoolIndex);
    constantPoolIndex += CONSTANT_POOL_LENGTH_BYTE_SIZE;
    for (int i = 0; i < constantPoolSize; ++i) {
      unsigned char type =
          ((unsigned char *)buf)[constantPoolIndex + CONSTANT_POLL_INDEX_BYTE_SIZE /*pool index*/];
      constantPoolIndex += CONSTANT_POLL_INDEX_BYTE_SIZE /*poll index*/
                           + BASIC_TYPE_BYTE_SIZE /*type*/ + getByteSizeFromType(type);
    }

    /**
     * u2 Number of static fields:
         ID
         static field name string ID
         u1
         type of field: (See Basic Type)
         value
         value of entry (u1, u2, u4, or u8 based on type of field)
     */

    int staticFieldsIndex = constantPoolIndex;
    int staticFieldsSize = getShortFromBytes((unsigned char *)buf, staticFieldsIndex);
    staticFieldsIndex += STATIC_FIELD_LENGTH_BYTE_SIZE;
    for (int i = 0; i < staticFieldsSize; ++i) {
      unsigned char type = ((unsigned char *)buf)[staticFieldsIndex + STRING_ID_BYTE_SIZE /*ID*/];
      staticFieldsIndex += STRING_ID_BYTE_SIZE /*string ID*/ + BASIC_TYPE_BYTE_SIZE /*type*/
                           + getByteSizeFromType(type);
    }

    /**
     * u2
       Number of instance fields (not including super class's)
            ID
            field name string ID
            u1
            type of field: (See Basic Type)
     */
    int instanceFieldsIndex = staticFieldsIndex;
    int instanceFieldsSize = getShortFromBytes((unsigned char *)buf, instanceFieldsIndex);
    instanceFieldsIndex += INSTANCE_FIELD_LENGTH_BYTE_SIZE;
    instanceFieldsIndex += (BASIC_TYPE_BYTE_SIZE + STRING_ID_BYTE_SIZE) * instanceFieldsSize;

    arraySerialNo = processHeap(buf, instanceFieldsIndex, maxLen, heapSerialNo, arraySerialNo);
  }

  break;

    /**
     *__ AddU1(HPROF_INSTANCE_DUMP);
     * __ AddObjectId(obj);
     * __ AddStackTraceSerialNumber(LookupStackTraceSerialNumber(obj));
     * __ AddClassId(LookupClassId(klass));
     *
     * __ AddU4(0x77777777);//length
     *
     * ***
     */
  case HPROF_INSTANCE_DUMP: {
    int instanceDumpIndex = firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE +
                            STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE + CLASS_ID_BYTE_SIZE;
    int instanceSize = getIntFromBytes((unsigned char *)buf, instanceDumpIndex);

    //裁剪掉system space
    if (isCurrentSystemHeap) {
      stripIndexListPair[stripIndex * 2] = firstIndex;
      stripIndexListPair[stripIndex * 2 + 1] = instanceDumpIndex + U4 /*占位*/ + instanceSize;
      stripIndex++;

      stripBytesSum += instanceDumpIndex + U4 /*占位*/ + instanceSize - firstIndex;
    }

    arraySerialNo = processHeap(buf, instanceDumpIndex + U4 /*占位*/ + instanceSize, maxLen,
                                heapSerialNo, arraySerialNo);
  } break;

    /**
     * __ AddU1(HPROF_OBJECT_ARRAY_DUMP);
     * __ AddObjectId(obj);
     * __ AddStackTraceSerialNumber(LookupStackTraceSerialNumber(obj));
     * __ AddU4(length);
     * __ AddClassId(LookupClassId(klass));
     *
     * // Dump the elements, which are always objects or null.
     * __ AddIdList(obj->AsObjectArray<mirror::Object>().Ptr());
     */
  case HPROF_OBJECT_ARRAY_DUMP: {
    int length = getIntFromBytes((unsigned char *)buf, firstIndex + HEAP_TAG_BYTE_SIZE +
                                                           OBJECT_ID_BYTE_SIZE +
                                                           STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE);

    //裁剪掉system space
    if (isCurrentSystemHeap) {
      stripIndexListPair[stripIndex * 2] = firstIndex;
      stripIndexListPair[stripIndex * 2 + 1] = firstIndex + HEAP_TAG_BYTE_SIZE +
                                               OBJECT_ID_BYTE_SIZE +
                                               STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE + U4 /*Length*/
                                               + CLASS_ID_BYTE_SIZE + U4 /*Id*/ * length;
      stripIndex++;

      stripBytesSum += HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE +
                       STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE + U4 /*Length*/
                       + CLASS_ID_BYTE_SIZE + U4 /*Id*/ * length;
    }

    arraySerialNo = processHeap(buf,
                                firstIndex + HEAP_TAG_BYTE_SIZE + OBJECT_ID_BYTE_SIZE +
                                    STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE + U4 /*Length*/
                                    + CLASS_ID_BYTE_SIZE + U4 /*Id*/ * length,
                                maxLen, heapSerialNo, arraySerialNo);
  } break;

    /**
     *
     * __ AddU1(HPROF_PRIMITIVE_ARRAY_DUMP);
     * __ AddClassStaticsId(klass);
     * __ AddStackTraceSerialNumber(LookupStackTraceSerialNumber(klass));
     * __ AddU4(java_heap_overhead_size - 4);
     * __ AddU1(hprof_basic_byte);
     * for (size_t i = 0; i < java_heap_overhead_size - 4; ++i) {
     *      __ AddU1(0);
     * }
     *
     * // obj is a primitive array.
     * __ AddU1(HPROF_PRIMITIVE_ARRAY_DUMP);
     * __ AddObjectId(obj);
     * __ AddStackTraceSerialNumber(LookupStackTraceSerialNumber(obj));
     * __ AddU4(length);
     * __ AddU1(t);
     * // Dump the raw, packed element values.
     * if (size == 1) {
     *      __ AddU1List(reinterpret_cast<const uint8_t*>(obj->GetRawData(sizeof(uint8_t), 0)),
     * length); } else if (size == 2) {
     *      __ AddU2List(reinterpret_cast<const uint16_t*>(obj->GetRawData(sizeof(uint16_t), 0)),
     * length); } else if (size == 4) {
     *      __ AddU4List(reinterpret_cast<const uint32_t*>(obj->GetRawData(sizeof(uint32_t), 0)),
     * length); } else if (size == 8) {
     *      __ AddU8List(reinterpret_cast<const uint64_t*>(obj->GetRawData(sizeof(uint64_t), 0)),
     * length);
     * }
     */
  case HPROF_PRIMITIVE_ARRAY_DUMP: {
    int primitiveArrayDumpIndex = firstIndex + HEAP_TAG_BYTE_SIZE /*tag*/
                                  + OBJECT_ID_BYTE_SIZE + STACK_TRACE_SERIAL_NUMBER_BYTE_SIZE;
    int length = getIntFromBytes((unsigned char *)buf, primitiveArrayDumpIndex);
    primitiveArrayDumpIndex += U4 /*Length*/;

    //裁剪掉基本类型数组，无论是否在system space都进行裁剪
    //区别是数组左坐标，app space时带数组元信息（类型、长度）方便回填
    if (isCurrentSystemHeap) {
      stripIndexListPair[stripIndex * 2] = firstIndex;
    } else {
      stripIndexListPair[stripIndex * 2] =
          primitiveArrayDumpIndex + BASIC_TYPE_BYTE_SIZE /*value type*/;
    }
    arraySerialNo++;

    int valueSize = getByteSizeFromType(((unsigned char *)buf)[primitiveArrayDumpIndex]);
    primitiveArrayDumpIndex += BASIC_TYPE_BYTE_SIZE /*value type*/ + valueSize * length;

    //数组右坐标
    stripIndexListPair[stripIndex * 2 + 1] = primitiveArrayDumpIndex;

    // app space时，不修改长度因为回填数组时会补齐
    if (isCurrentSystemHeap) {
      stripBytesSum += primitiveArrayDumpIndex - firstIndex;
    }
    stripIndex++;

    arraySerialNo = processHeap(buf, primitiveArrayDumpIndex, maxLen, heapSerialNo, arraySerialNo);
  } break;

    // Android.
  case HPROF_HEAP_DUMP_INFO: {
    const unsigned char heapType = ((unsigned char *)buf)[firstIndex + HEAP_TAG_BYTE_SIZE + 3];
    isCurrentSystemHeap = (heapType == HPROF_HEAP_ZYGOTE || heapType == HPROF_HEAP_IMAGE);

    if (isCurrentSystemHeap) {
      stripIndexListPair[stripIndex * 2] = firstIndex;
      stripIndexListPair[stripIndex * 2 + 1] = firstIndex + HEAP_TAG_BYTE_SIZE /*TAG*/
                                               + HEAP_TYPE_BYTE_SIZE           /*heap type*/
                                               + STRING_ID_BYTE_SIZE /*string id*/;
      stripIndex++;
      stripBytesSum += HEAP_TAG_BYTE_SIZE    /*TAG*/
                       + HEAP_TYPE_BYTE_SIZE /*heap type*/
                       + STRING_ID_BYTE_SIZE /*string id*/;
    }

    arraySerialNo = processHeap(buf,
                                firstIndex + HEAP_TAG_BYTE_SIZE /*TAG*/
                                    + HEAP_TYPE_BYTE_SIZE       /*heap type*/
                                    + STRING_ID_BYTE_SIZE /*string id*/,
                                maxLen, heapSerialNo, arraySerialNo);
  } break;

  case HPROF_ROOT_FINALIZING:               // Obsolete.
  case HPROF_ROOT_REFERENCE_CLEANUP:        // Obsolete.
  case HPROF_UNREACHABLE:                   // Obsolete.
  case HPROF_PRIMITIVE_ARRAY_NODATA_DUMP: { // Obsolete.
    arraySerialNo =
        processHeap(buf, firstIndex + HEAP_TAG_BYTE_SIZE, maxLen, heapSerialNo, arraySerialNo);
  } break;

  default:
    break;
  }
  return arraySerialNo;
}

void reset() {
  stripIndex = 0;
  stripBytesSum = 0;
}

ssize_t hook_write(int fd, const void *buf, size_t count) {
  if (fd != hprofFd) {
    ssize_t total_write = write(fd, buf, count);
    return total_write;
  }

  //每次hook_write，初始化重置
  reset();

  const unsigned char tag = ((unsigned char *)buf)[0];
  //删除掉无关record tag类型匹配，只匹配heap相关提高性能
  switch (tag) {
  case HPROF_TAG_HEAP_DUMP:
  case HPROF_TAG_HEAP_DUMP_SEGMENT: {
    processHeap(buf, HEAP_TAG_BYTE_SIZE + RECORD_TIME_BYTE_SIZE + RECORD_LENGTH_BYTE_SIZE, count,
                heapSerialNum, 0);
    heapSerialNum++;
  } break;
  default:
    break;
  }

  //根据裁剪掉的zygote space和image space更新length
  int recordLength = 0;
  if (tag == HPROF_TAG_HEAP_DUMP || tag == HPROF_TAG_HEAP_DUMP_SEGMENT) {
    recordLength =
        getIntFromBytes((unsigned char *)buf, HEAP_TAG_BYTE_SIZE + RECORD_TIME_BYTE_SIZE);
    recordLength -= stripBytesSum;
    int index = HEAP_TAG_BYTE_SIZE + RECORD_TIME_BYTE_SIZE;
    ((unsigned char *)buf)[index] =
        (unsigned char)(((unsigned int)recordLength & 0xff000000u) >> 24u);
    ((unsigned char *)buf)[index + 1] =
        (unsigned char)(((unsigned int)recordLength & 0x00ff0000u) >> 16u);
    ((unsigned char *)buf)[index + 2] =
        (unsigned char)(((unsigned int)recordLength & 0x0000ff00u) >> 8u);
    ((unsigned char *)buf)[index + 3] = (unsigned char)((unsigned int)recordLength & 0x000000ffu);
  }

  ssize_t total_write = 0;
  int startIndex = 0;
  for (int i = 0; i < stripIndex; i++) {
    //将裁剪掉的区间，通过写时过滤掉
    void *writeBuf = (void *)((unsigned char *)buf + startIndex);
    auto writeLen = (size_t)(stripIndexListPair[i * 2] - startIndex);
    if (writeLen > 0) {
      total_write += write(fd, writeBuf, writeLen);
    } else if (writeLen < 0) {
      __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "hook_write array i:%d writeLen<0:%lu", i,
                          writeLen);
    }
    startIndex = stripIndexListPair[i * 2 + 1];
  }
  auto writeLen = (size_t)(count - startIndex);
  if (writeLen > 0) {
    void *writeBuf = (void *)((unsigned char *)buf + startIndex);
    total_write += write(fd, writeBuf, count - startIndex);
  }

  hookWriteSerialNum++;

  if (total_write != count) {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "hook write, hprof strip happens");
  }

  return count;
}

void (*suspendVM)();
void (*resumeVM)();

bool initForkVMSymbols() {
  void *libHandle = kwai::linker::DlFcn::dlopen("libart.so", RTLD_NOW);
  if (libHandle == nullptr) {
    return false;
  }

  suspendVM = (void (*)())kwai::linker::DlFcn::dlsym(libHandle, "_ZN3art3Dbg9SuspendVMEv");
  if (suspendVM == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art3Dbg9SuspendVMEv unsupported!");
  }

  resumeVM = (void (*)())kwai::linker::DlFcn::dlsym(libHandle, "_ZN3art3Dbg8ResumeVMEv");
  if (resumeVM == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art3Dbg8ResumeVMEv unsupported!");
  }

  kwai::linker::DlFcn::dlclose(libHandle);
  return suspendVM != nullptr && resumeVM != nullptr;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_com_kwai_koom_javaoom_dump_StripHprofHeapDumper_initStripDump(JNIEnv *env, jobject jObject) {
  hprofFd = -1;
  hprofName = nullptr;
  isDumpHookSucc = false;

  xhook_enable_debug(0);

  /**
   *
   * android 7.x，write方法在libc.so中
   * android 8-9，write方法在libart.so中
   * android 10，write方法在libartbase.so中
   * libbase.so是一个保险操作，防止前面2个so里面都hook不到(:
   *
   * android 7-10版本，open方法都在libart.so中
   * libbase.so与libartbase.so，为保险操作
   */
  xhook_register("libart.so", "open", (void *)hook_open, nullptr);
  xhook_register("libbase.so", "open", (void *)hook_open, nullptr);
  xhook_register("libartbase.so", "open", (void *)hook_open, nullptr);

  xhook_register("libc.so", "write", (void *)hook_write, nullptr);
  xhook_register("libart.so", "write", (void *)hook_write, nullptr);
  xhook_register("libbase.so", "write", (void *)hook_write, nullptr);
  xhook_register("libartbase.so", "write", (void *)hook_write, nullptr);

  xhook_refresh(0);
  xhook_clear();
}

JNIEXPORT void JNICALL Java_com_kwai_koom_javaoom_dump_StripHprofHeapDumper_hprofName(
    JNIEnv *env, jobject jObject, jstring name) {
  hprofName = (char *)env->GetStringUTFChars(name, (jboolean *)false);
}

JNIEXPORT jboolean JNICALL
Java_com_kwai_koom_javaoom_dump_StripHprofHeapDumper_isStripSuccess(JNIEnv *env, jobject jObject) {
  return (jboolean)isDumpHookSucc;
}

static void initDumpHprofSymbols();
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

JNIEXPORT void JNICALL
Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_initForkDump(JNIEnv *env, jobject jObject) {
  if (!initForkVMSymbols()) {
    // Above android 11
    pthread_once(&once_control, initDumpHprofSymbols);
  }
}

JNIEXPORT jint JNICALL Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_fork(JNIEnv *env,
                                                                              jobject jObject) {
  return fork();
}

JNIEXPORT jint JNICALL Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_trySuspendVMThenFork(
    JNIEnv *env, jobject jObject) {
  if (suspendVM == nullptr) {
    initForkVMSymbols();
  }
  if (suspendVM != nullptr) {
    suspendVM();
  }
  return fork();
}

JNIEXPORT void JNICALL
Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_suspendVM(JNIEnv *env, jobject jObject) {
  if (suspendVM == nullptr) {
    initForkVMSymbols();
  }

  if (suspendVM != nullptr) {
    suspendVM();
  }
}

JNIEXPORT void JNICALL Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_resumeVM(JNIEnv *env,
                                                                                  jobject jObject) {
  if (resumeVM == nullptr) {
    initForkVMSymbols();
  }
  if (resumeVM != nullptr) {
    resumeVM();
  }
}

JNIEXPORT void JNICALL
Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_exitProcess(JNIEnv *env, jobject jObject) {
  _exit(0);
}

JNIEXPORT void JNICALL Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_waitPid(JNIEnv *env,
                                                                                 jobject jObject,
                                                                                 jint pid) {
  int status;
  waitpid(pid, &status, 0);
}

#define TLS_SLOT_ART_THREAD_SELF 7
#if defined(__aarch64__)
#define __get_tls()                                                                                \
  ({                                                                                               \
    void **__val;                                                                                  \
    __asm__("mrs %0, tpidr_el0" : "=r"(__val));                                                    \
    __val;                                                                                         \
  })
#elif defined(__arm__)
#define __get_tls()                                                                                \
  ({                                                                                               \
    void **__val;                                                                                  \
    __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(__val));                                           \
    __val;                                                                                         \
  })
#else
#error unsupported architecture
#endif

// What caused the GC?
enum GcCause {
  // Invalid GC cause used as a placeholder.
  kGcCauseNone,
  // GC triggered by a failed allocation. Thread doing allocation is blocked waiting for GC before
  // retrying allocation.
  kGcCauseForAlloc,
  // A background GC trying to ensure there is free memory ahead of allocations.
  kGcCauseBackground,
  // An explicit System.gc() call.
  kGcCauseExplicit,
  // GC triggered for a native allocation when NativeAllocationGcWatermark is exceeded.
  // (This may be a blocking GC depending on whether we run a non-concurrent collector).
  kGcCauseForNativeAlloc,
  // GC triggered for a collector transition.
  kGcCauseCollectorTransition,
  // Not a real GC cause, used when we disable moving GC (currently for GetPrimitiveArrayCritical).
  kGcCauseDisableMovingGc,
  // Not a real GC cause, used when we trim the heap.
  kGcCauseTrim,
  // Not a real GC cause, used to implement exclusion between GC and instrumentation.
  kGcCauseInstrumentation,
  // Not a real GC cause, used to add or remove app image spaces.
  kGcCauseAddRemoveAppImageSpace,
  // Not a real GC cause, used to implement exclusion between GC and debugger.
  kGcCauseDebugger,
  // GC triggered for background transition when both foreground and background collector are CMS.
  kGcCauseHomogeneousSpaceCompact,
  // Class linker cause, used to guard filling art methods with special values.
  kGcCauseClassLinker,
  // Not a real GC cause, used to implement exclusion between code cache metadata and GC.
  kGcCauseJitCodeCache,
  // Not a real GC cause, used to add or remove system-weak holders.
  kGcCauseAddRemoveSystemWeakHolder,
  // Not a real GC cause, used to prevent hprof running in the middle of GC.
  kGcCauseHprof,
  // Not a real GC cause, used to prevent GetObjectsAllocated running in the middle of GC.
  kGcCauseGetObjectsAllocated,
  // GC cause for the profile saver.
  kGcCauseProfileSaver,
  // GC cause for running an empty checkpoint.
  kGcCauseRunEmptyCheckpoint,
};

// Which types of collections are able to be performed.
enum CollectorType {
  // No collector selected.
  kCollectorTypeNone,
  // Non concurrent mark-sweep.
  kCollectorTypeMS,
  // Concurrent mark-sweep.
  kCollectorTypeCMS,
  // Semi-space / mark-sweep hybrid, enables compaction.
  kCollectorTypeSS,
  // Heap trimming collector, doesn't do any actual collecting.
  kCollectorTypeHeapTrim,
  // A (mostly) concurrent copying collector.
  kCollectorTypeCC,
  // The background compaction of the concurrent copying collector.
  kCollectorTypeCCBackground,
  // Instrumentation critical section fake collector.
  kCollectorTypeInstrumentation,
  // Fake collector for adding or removing application image spaces.
  kCollectorTypeAddRemoveAppImageSpace,
  // Fake collector used to implement exclusion between GC and debugger.
  kCollectorTypeDebugger,
  // A homogeneous space compaction collector used in background transition
  // when both foreground and background collector are CMS.
  kCollectorTypeHomogeneousSpaceCompact,
  // Class linker fake collector.
  kCollectorTypeClassLinker,
  // JIT Code cache fake collector.
  kCollectorTypeJitCodeCache,
  // Hprof fake collector.
  kCollectorTypeHprof,
  // Fake collector for installing/removing a system-weak holder.
  kCollectorTypeAddRemoveSystemWeakHolder,
  // Fake collector type for GetObjectsAllocated
  kCollectorTypeGetObjectsAllocated,
  // Fake collector type for ScopedGCCriticalSection
  kCollectorTypeCriticalSection,
};

// Over size malloc ScopedSuspendAll instance for device compatibility
static void *gSSAHandle = malloc(64);
void (*ScopedSuspendAllConstructor)(void *handle, const char *cause, bool long_suspend);
void (*ScopedSuspendAllDestructor)(void *handle);
// Over size malloc ScopedGCCriticalSection instance for device compatibility
static void *gSGCSHandle = malloc(64);
void (*ScopedGCCriticalSectionConstructor)(void *handle, void *self, GcCause cause,
                                           CollectorType collector_type);
void (*ScopedGCCriticalSectionDestructor)(void *handle);
// Over size malloc Hprof instance for device compatibility
static void *gHprofHandle = malloc(128);
void (*HprofConstructor)(void *handle, const char *output_filename, int fd, bool direct_to_ddms);
void (*HprofDestructor)(void *handle);
void (*Dump)(void *handle);

// For above android 11
static void initDumpHprofSymbols() {
  // Parse .dynsym(GLOBAL)
  void *libHandle = kwai::linker::DlFcn::dlopen("libart.so", RTLD_NOW);
  if (libHandle == nullptr) {
    return;
  }
  ScopedSuspendAllConstructor = (void (*)(void *, const char *, bool))kwai::linker::DlFcn::dlsym(
      libHandle, "_ZN3art16ScopedSuspendAllC1EPKcb");
  if (ScopedSuspendAllConstructor == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art16ScopedSuspendAllC1EPKcb unsupported!");
  }

  ScopedSuspendAllDestructor =
      (void (*)(void *))kwai::linker::DlFcn::dlsym(libHandle, "_ZN3art16ScopedSuspendAllD1Ev");
  if (ScopedSuspendAllDestructor == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art16ScopedSuspendAllD1Ev unsupported!");
  }

  ScopedGCCriticalSectionConstructor =
      (void (*)(void *, void *, GcCause, CollectorType))kwai::linker::DlFcn::dlsym(
          libHandle,
          "_ZN3art2gc23ScopedGCCriticalSectionC1EPNS_6ThreadENS0_7GcCauseENS0_13CollectorTypeE");
  if (ScopedGCCriticalSectionConstructor == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG,
                        "_ZN3art2gc23ScopedGCCriticalSectionC1EPNS_6ThreadENS0_7GcCauseENS0_"
                        "13CollectorTypeE unsupported!");
  }

  ScopedGCCriticalSectionDestructor = (void (*)(void *))kwai::linker::DlFcn::dlsym(
      libHandle, "_ZN3art2gc23ScopedGCCriticalSectionD1Ev");
  if (ScopedGCCriticalSectionDestructor == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG,
                        "_ZN3art2gc23ScopedGCCriticalSectionD1Ev unsupported!");
  }

  kwai::linker::DlFcn::dlclose(libHandle);
  // Parse .symtab(LOCAL)
  libHandle = kwai::linker::DlFcn::dlopen_elf("libart.so", RTLD_NOW);
  if (libHandle == nullptr) {
    return;
  }
  HprofConstructor = (void (*)(void *, const char *, int, bool))kwai::linker::DlFcn::dlsym_elf(
      libHandle, "_ZN3art5hprof5HprofC2EPKcib");
  if (HprofConstructor == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art5hprof5HprofC2EPKcib unsupported!");
  }

  HprofDestructor =
      (void (*)(void *))kwai::linker::DlFcn::dlsym_elf(libHandle, "_ZN3art5hprof5HprofD0Ev");
  if (HprofDestructor == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art5hprof5HprofD0Ev unsupported!");
  }

  Dump = (void (*)(void *))kwai::linker::DlFcn::dlsym_elf(libHandle, "_ZN3art5hprof5Hprof4DumpEv");
  if (Dump == nullptr) {
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "_ZN3art5hprof5Hprof4DumpEv unsupported!");
  }

  kwai::linker::DlFcn::dlclose_elf(libHandle);
}

JNIEXPORT jboolean JNICALL Java_com_kwai_koom_javaoom_dump_ForkJvmHeapDumper_dumpHprofDataNative(
    JNIEnv *env, jclass clazz, jstring file_name) {
  pthread_once(&once_control, initDumpHprofSymbols);
  if (ScopedGCCriticalSectionConstructor == nullptr || ScopedSuspendAllConstructor == nullptr ||
      ScopedGCCriticalSectionDestructor == nullptr || ScopedSuspendAllDestructor == nullptr ||
      HprofConstructor == nullptr || HprofDestructor == nullptr || Dump == nullptr) {
    return JNI_FALSE;
  }
  ScopedGCCriticalSectionConstructor(gSGCSHandle, __get_tls()[TLS_SLOT_ART_THREAD_SELF],
                                     kGcCauseHprof, kCollectorTypeHprof);
  ScopedSuspendAllConstructor(gSSAHandle, LOG_TAG, true);
  pid_t pid = fork();
  if (pid == -1) {
    // Fork error.
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "failed to fork!");
    return JNI_FALSE;
  }
  if (pid != 0) {
    // Parent
    ScopedGCCriticalSectionDestructor(gSGCSHandle);
    ScopedSuspendAllDestructor(gSSAHandle);

    int stat_loc;
    for (;;) {
      if (waitpid(pid, &stat_loc, 0) != -1 || errno != EINTR) {
        break;
      }
    }
    return JNI_TRUE;
  }

  const char *filename = env->GetStringUTFChars(file_name, nullptr);
  HprofConstructor(gHprofHandle, filename, -1, false);
  Dump(gHprofHandle);
  HprofDestructor(gHprofHandle);
  env->ReleaseStringUTFChars(file_name, filename);
  return JNI_TRUE;
}

#ifdef __cplusplus
}
#endif
