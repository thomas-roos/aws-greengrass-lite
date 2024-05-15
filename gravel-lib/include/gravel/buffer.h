/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_BUFFER_H
#define GRAVEL_BUFFER_H

/*! Map utilities */

#include "object.h"
#include <stdbool.h>

/** Get the value corresponding with a key.
 * If not found, returns false and `result` is NULL. */
bool gravel_buffer_eq(GravelBuffer buf1, GravelBuffer buf2);

#endif
