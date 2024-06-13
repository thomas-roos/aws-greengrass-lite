/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GRAVEL_MAP_H
#define GRAVEL_MAP_H

/*! Map utilities */

#include "object.h"
#include <stdbool.h>

// NOLINTBEGIN(bugprone-macro-parentheses)
/** Loop over the KV pairs in a map. */
#define GRAVEL_MAP_FOREACH(name, map) \
    for (GravelKV *name = (map).pairs; name < &(map).pairs[(map).len]; \
         name = &name[1])
// NOLINTEND(bugprone-macro-parentheses)

/** Get the value corresponding with a key.
 * Returns whether the key was found in the map.
 * If `result` is not NULL it is set to the found value or NULL. */
bool gravel_map_get(GravelMap map, GravelBuffer key, GravelObject **result);

#endif
