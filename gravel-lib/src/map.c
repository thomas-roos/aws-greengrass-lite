/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gravel/map.h"
#include "gravel/buffer.h"
#include "gravel/object.h"
#include <stdbool.h>
#include <stdlib.h>

bool gravel_map_get(GravelMap map, GravelBuffer key, GravelObject **result) {
    GRAVEL_MAP_FOREACH(pair, map) {
        if (gravel_buffer_eq(key, pair->key)) {
            if (result != NULL) {
                *result = &pair->val;
            }
            return true;
        }
    }
    if (result != NULL) {
        *result = NULL;
    }
    return false;
}
