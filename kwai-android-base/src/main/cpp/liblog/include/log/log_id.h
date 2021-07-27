// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#pragma once

#include <android/klog.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * log_id_t helpers
 */
log_id_t android_name_to_log_id(const char *logName);
const char *android_log_id_to_name(log_id_t log_id);

#ifdef __cplusplus
}
#endif