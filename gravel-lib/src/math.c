/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/math.h"
#include <stdint.h>

uint64_t gravel_abs(int64_t i64) {
    if (i64 >= 0) {
        return (uint64_t) i64;
    }
    if (i64 >= INT32_MIN) {
        // Handling small values so next step can divide without 0 result
        return (uint64_t) -i64;
    }
    // Avoiding assuming twos-compliment.
    // Using division to split into two negatable values and recombine. */
    return (uint64_t) (i64 / INT32_MIN) * (uint64_t) (-(int64_t) INT32_MIN)
        + (uint64_t) (-(i64 % INT32_MIN));
}
