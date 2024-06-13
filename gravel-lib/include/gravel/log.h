/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GRAVEL_LOG_H
#define GRAVEL_LOG_H

/*!  logging interface */

#include <stdint.h>

/** Logging interface implementation.
 * Do not call directly; use one of the macro wrappers.
 * Can be overridden by providing a non-weak definition.
 * Default implementation prints to stderr. */
void gravel_log(
    uint32_t level,
    const char *file,
    int line,
    const char *tag,
    const char *format,
    ...
) __attribute__((format(printf, 5, 6)));

/** No-op logging fn for enabling type checking disabled logging macros. */
__attribute__((format(printf, 2, 3), always_inline)) static inline void
gravel_log_disabled(const char *tag, const char *format, ...) {
    (void) tag;
    (void) format;
}

#define GRAVEL_LOG_NONE 0
#define GRAVEL_LOG_ERROR 1
#define GRAVEL_LOG_WARN 2
#define GRAVEL_LOG_INFO 3
#define GRAVEL_LOG_DEBUG 4
#define GRAVEL_LOG_TRACE 5

/** Minimum log level to print.
 * Can be overridden from make using command line or environment. */
#ifndef GRAVEL_LOG_LEVEL
#define GRAVEL_LOG_LEVEL GRAVEL_LOG_INFO
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define GRAVEL_LOG(level, ...) \
    gravel_log(level, __FILE_NAME__, __LINE__, __VA_ARGS__)

#if GRAVEL_LOG_LEVEL >= GRAVEL_LOG_ERROR
#define GRAVEL_LOGE(...) GRAVEL_LOG(GRAVEL_LOG_ERROR, __VA_ARGS__)
#else
#define GRAVEL_LOGE(...) gravel_log_disabled(__VA_ARGS__)
#endif

#if GRAVEL_LOG_LEVEL >= GRAVEL_LOG_WARN
#define GRAVEL_LOGW(...) GRAVEL_LOG(GRAVEL_LOG_WARN, __VA_ARGS__)
#else
#define GRAVEL_LOGW(...) gravel_log_disabled(__VA_ARGS__)
#endif

#if GRAVEL_LOG_LEVEL >= GRAVEL_LOG_INFO
#define GRAVEL_LOGI(...) GRAVEL_LOG(GRAVEL_LOG_INFO, __VA_ARGS__)
#else
#define GRAVEL_LOGI(...) gravel_log_disabled(__VA_ARGS__)
#endif

#if GRAVEL_LOG_LEVEL >= GRAVEL_LOG_DEBUG
#define GRAVEL_LOGD(...) GRAVEL_LOG(GRAVEL_LOG_DEBUG, __VA_ARGS__)
#else
#define GRAVEL_LOGD(...) gravel_log_disabled(__VA_ARGS__)
#endif

#if GRAVEL_LOG_LEVEL >= GRAVEL_LOG_TRACE
#define GRAVEL_LOGT(...) GRAVEL_LOG(GRAVEL_LOG_TRACE, __VA_ARGS__)
#else
#define GRAVEL_LOGT(...) gravel_log_disabled(__VA_ARGS__)
#endif

#endif
