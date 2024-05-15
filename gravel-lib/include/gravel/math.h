/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_MATH_H
#define GRAVEL_MATH_H

/*! Math utilites */

#include <stdint.h>

/** Absolute value, avoiding undefined behavior.
 * i.e. avoiding -INT64_MIN for twos-compliment) */
uint64_t gravel_abs(int64_t i64);

#endif
