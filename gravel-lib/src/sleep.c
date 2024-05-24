/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/log.h"
#include "gravel/utils.h"
#include <errno.h>
#include <time.h>
#include <stdint.h>

int gravel_sleep(int64_t seconds) {
    struct timespec time = { .tv_sec = seconds };
    struct timespec remain = { 0 };

    while (nanosleep(&time, &remain) != 0) {
        if (errno != EINTR) {
            int err = errno;
            GRAVEL_LOGE("mqtt", "nanosleep failed: %d.", err);
            return err;
        }

        time = remain;
    }

    return 0;
}
