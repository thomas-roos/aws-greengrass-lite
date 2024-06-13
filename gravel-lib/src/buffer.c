/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gravel/buffer.h"
#include "gravel/object.h"
#include <string.h>
#include <stdbool.h>

bool gravel_buffer_eq(GravelBuffer buf1, GravelBuffer buf2) {
    if (buf1.len == buf2.len) {
        return memcmp(buf1.data, buf2.data, buf1.len) == 0;
    }
    return false;
}
