/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gravel/log.h"
#include "gravel/defer.h"
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

__attribute__((weak)) void gravel_log(
    uint32_t level,
    const char *file,
    int line,
    const char *tag,
    const char *format,
    ...
) {
    int errno_old = errno;

    char *level_str;
    switch (level) {
    case GRAVEL_LOG_ERROR: level_str = "\033[1;31mE"; break;
    case GRAVEL_LOG_WARN: level_str = "\033[1;33mW"; break;
    case GRAVEL_LOG_INFO: level_str = "\033[0;32mI"; break;
    case GRAVEL_LOG_DEBUG: level_str = "\033[0;34mD"; break;
    case GRAVEL_LOG_TRACE: level_str = "\033[0;37mT"; break;
    default: level_str = "\033[0;37m?";
    }

    {
        pthread_mutex_lock(&log_mutex);
        GRAVEL_DEFER(pthread_mutex_unlock, log_mutex);

        fprintf(stderr, "%s[%s] %s:%d: ", level_str, tag, file, line);

        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        fprintf(stderr, "\033[0m\n");
    }

    errno = errno_old;
}
