/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/buffer.h"
#include <string.h>
#include <stdbool.h>

bool gravel_buffer_eq(GravelBuffer buf1, GravelBuffer buf2) {
    if (buf1.len == buf2.len) {
        return memcmp(buf1.data, buf2.data, buf1.len) == 0;
    }
    return false;
}
