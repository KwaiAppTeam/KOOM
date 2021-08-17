/*
 * Copyright (c) 2020. Kwai, Inc. All rights reserved.
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
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2020.
 *
 */

#pragma once

#include <sys/cdefs.h>
#include <sys/system_properties.h>
#include <sys/types.h>

__BEGIN_DECLS

#if defined(__USE_GNU) && !defined(basename)
/*
 * glibc has a basename in <string.h> that's different to the POSIX one in <libgen.h>.
 * It doesn't modify its argument, and in C++ it's const-correct.
 */
#if defined(__cplusplus)
extern "C++" char *basename(char *__path) __RENAME(__gnu_basename);
extern "C++" const char *basename(const char *__path) __RENAME(__gnu_basename);
#else
char *basename(const char *__path) __RENAME(__gnu_basename);
#endif
#endif

ssize_t kwai_process_vm_readv(pid_t pid, const struct iovec *lvec, unsigned long liovcnt,
                              const struct iovec *rvec, unsigned long riovcnt, unsigned long flags);
ssize_t kwai_process_vm_writev(pid_t pid, const struct iovec *lvec, unsigned long liovcnt,
                               const struct iovec *rvec, unsigned long riovcnt,
                               unsigned long flags);

void kwai_set_abort_message(const char *__msg);

const char *kwai_getprogname(void);

void kwai__system_property_read_callback(const prop_info *info,
                                         void (*callback)(void *cookie, const char *name,
                                                          const char *value, uint32_t serial),
                                         void *cookie);
bool kwai__system_property_wait(const prop_info *__pi, uint32_t __old_serial,
                                uint32_t *__new_serial_ptr,
                                const struct timespec *__relative_timeout);

uint32_t kwai__system_property_area_serial(void);
uint32_t kwai__system_property_serial(const prop_info *__pi);

__END_DECLS