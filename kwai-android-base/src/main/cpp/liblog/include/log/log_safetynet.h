// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define android_errorWriteLog(tag, subTag) __android_log_error_write(tag, subTag, -1, NULL, 0)

#define android_errorWriteWithInfoLog(tag, subTag, uid, data, dataLen)                             \
  __android_log_error_write(tag, subTag, uid, data, dataLen)

int __android_log_error_write(int tag, const char *subTag, int32_t uid, const char *data,
                              uint32_t dataLen);

#ifdef __cplusplus
}
#endif