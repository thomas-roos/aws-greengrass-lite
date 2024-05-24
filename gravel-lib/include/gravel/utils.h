/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_UTILS_H
#define GRAVEL_UTILS_H

/*! Misc utilites */

#include <stdint.h>

/** Sleep for given duration in seconds.
 * Returns 0 on success else error code. */

int gravel_sleep(int64_t seconds);

#endif
