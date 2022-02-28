/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#pragma once

/**
 * @addtogroup Logging
 * @{
 */

/**
 * \file
 *
 * Support routines to send messages to the Android log buffer,
 * which can later be accessed through the `logcat` utility.
 *
 * Each log message must have
 *   - a priority
 *   - a log tag
 *   - some text
 *
 * The tag normally corresponds to the component that emits the log message,
 * and should be reasonably small.
 *
 * Log message text may be truncated to less than an implementation-specific
 * limit (1023 bytes).
 *
 * Note that a newline character ("\n") will be appended automatically to your
 * log message, if not already there. It is not possible to send several
 * messages and have them appear on a single line in logcat.
 *
 * Please use logging in moderation:
 *
 *  - Sending log messages eats CPU and slow down your application and the
 *    system.
 *
 *  - The circular log buffer is pretty small, so sending many messages
 *    will hide other important log messages.
 *
 *  - In release builds, only send log messages to account for exceptional
 *    conditions.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

/**
 * Temporarily resolve compilation conflicts caused by __INTRODUCED_IN strict behavior change
 */
#ifdef __BIONIC_AVAILABILITY
#undef __BIONIC_AVAILABILITY
#define __BIONIC_AVAILABILITY(__what) __attribute__((__availability__(android,__what)))
#include <android/log.h>
#endif

#if !defined(__BIONIC__) && !defined(__INTRODUCED_IN)
#define __INTRODUCED_IN(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Remove duplicate with NDK
 */

/**
 * Writes the log message specified by log_message.  log_message includes additional file name and
 * line number information that a logger may use.  log_message is versioned for backwards
 * compatibility.
 * This assumes that loggability has already been checked through __android_log_is_loggable().
 * Higher level logging libraries, such as libbase, first check loggability, then format their
 * buffers, then pass the message to liblog via this function, and therefore we do not want to
 * duplicate the loggability check here.
 *
 * @param log_message the log message itself, see __android_log_message.
 *
 * Available since API level 30.
 */
void __android_log_write_log_message(struct __android_log_message *log_message);

/**
 * Sets a user defined logger function.  All log messages sent to liblog will be set to the
 * function pointer specified by logger for processing.  It is not expected that log messages are
 * already terminated with a new line.  This function should add new lines if required for line
 * separation.
 *
 * @param logger the new function that will handle log messages.
 *
 * Available since API level 30.
 */
void __android_log_set_logger(__android_logger_function logger);

/**
 * Writes the log message to logd.  This is an __android_logger_function and can be provided to
 * __android_log_set_logger().  It is the default logger when running liblog on a device.
 *
 * @param log_message the log message to write, see __android_log_message.
 *
 * Available since API level 30.
 */
void __android_log_logd_logger(const struct __android_log_message *log_message);

/**
 * Writes the log message to stderr.  This is an __android_logger_function and can be provided to
 * __android_log_set_logger().  It is the default logger when running liblog on host.
 *
 * @param log_message the log message to write, see __android_log_message.
 *
 * Available since API level 30.
 */
void __android_log_stderr_logger(const struct __android_log_message *log_message)
   ;

/**
 * Sets a user defined aborter function that is called for __android_log_assert() failures.  This
 * user defined aborter function is highly recommended to abort and be noreturn, but is not strictly
 * required to.
 *
 * @param aborter the new aborter function, see __android_aborter_function.
 *
 * Available since API level 30.
 */
void __android_log_set_aborter(__android_aborter_function aborter);

/**
 * Calls the stored aborter function.  This allows for other logging libraries to use the same
 * aborter function by calling this function in liblog.
 *
 * @param abort_message an additional message supplied when aborting, for example this is used to
 *                      call android_set_abort_message() in __android_log_default_aborter().
 *
 * Available since API level 30.
 */
void __android_log_call_aborter(const char *abort_message);

/**
 * Sets android_set_abort_message() on device then aborts().  This is the default aborter.
 *
 * @param abort_message an additional message supplied when aborting.  This functions calls
 *                      android_set_abort_message() with its contents.
 *
 * Available since API level 30.
 */
void __android_log_default_aborter(const char *abort_message) __attribute__((noreturn));

/**
 * Use the per-tag properties "log.tag.<tagname>" along with the minimum priority from
 * __android_log_set_minimum_priority() to determine if a log message with a given prio and tag will
 * be printed.  A non-zero result indicates yes, zero indicates false.
 *
 * If both a priority for a tag and a minimum priority are set by
 * __android_log_set_minimum_priority(), then the lowest of the two values are to determine the
 * minimum priority needed to log.  If only one is set, then that value is used to determine the
 * minimum priority needed.  If none are set, then default_priority is used.
 *
 * @param prio         the priority to test, takes android_LogPriority values.
 * @param tag          the tag to test.
 * @param default_prio the default priority to use if no properties or minimum priority are set.
 * @return an integer where 1 indicates that the message is loggable and 0 indicates that it is not.
 *
 * Available since API level 30.
 */
int __android_log_is_loggable(int prio, const char *tag, int default_prio);

/**
 * Use the per-tag properties "log.tag.<tagname>" along with the minimum priority from
 * __android_log_set_minimum_priority() to determine if a log message with a given prio and tag will
 * be printed.  A non-zero result indicates yes, zero indicates false.
 *
 * If both a priority for a tag and a minimum priority are set by
 * __android_log_set_minimum_priority(), then the lowest of the two values are to determine the
 * minimum priority needed to log.  If only one is set, then that value is used to determine the
 * minimum priority needed.  If none are set, then default_priority is used.
 *
 * @param prio         the priority to test, takes android_LogPriority values.
 * @param tag          the tag to test.
 * @param len          the length of the tag.
 * @param default_prio the default priority to use if no properties or minimum priority are set.
 * @return an integer where 1 indicates that the message is loggable and 0 indicates that it is not.
 *
 * Available since API level 30.
 */
int __android_log_is_loggable_len(int prio, const char *tag, size_t len, int default_prio);

/**
 * Sets the minimum priority that will be logged for this process.
 *
 * @param priority the new minimum priority to set, takes android_LogPriority values.
 * @return the previous set minimum priority as android_LogPriority values, or
 *         ANDROID_LOG_DEFAULT if none was set.
 *
 * Available since API level 30.
 */
int32_t __android_log_set_minimum_priority(int32_t priority);

/**
 * Gets the minimum priority that will be logged for this process.  If none has been set by a
 * previous __android_log_set_minimum_priority() call, this returns ANDROID_LOG_DEFAULT.
 *
 * @return the current minimum priority as android_LogPriority values, or
 *         ANDROID_LOG_DEFAULT if none is set.
 *
 * Available since API level 30.
 */
int32_t __android_log_get_minimum_priority(void);

/**
 * Sets the default tag if no tag is provided when writing a log message.  Defaults to
 * getprogname().  This truncates tag to the maximum log message size, though appropriate tags
 * should be much smaller.
 *
 * @param tag the new log tag.
 *
 * Available since API level 30.
 */
void __android_log_set_default_tag(const char *tag);

#ifdef __cplusplus
}
#endif

/** @} */