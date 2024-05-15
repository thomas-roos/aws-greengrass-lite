/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/map.h"
#include "gravel/buffer.h"
#include "gravel/object.h"
#include <stdbool.h>

bool gravel_map_get(GravelMap map, GravelBuffer key, GravelObject **result) {
    *result = NULL;
    GRAVEL_MAP_FOREACH(pair, map) {
        if (gravel_buffer_eq(key, pair->key)) {
            if (result != NULL) {
                *result = &pair->val;
            }
            return true;
        }
    }
    return false;
}