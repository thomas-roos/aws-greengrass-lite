/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_BUMP_ALLOC_H
#define GRAVEL_BUMP_ALLOC_H

/*! A simple bump allocator */

#include "alloc.h"
#include "object.h"
#include <stddef.h>

/** Alloc-only allocator backed by a fixed buffer. */
typedef struct {
    GravelAlloc alloc;
    GravelBuffer buf;
    size_t index;
} GravelBumpAlloc;

/** Obtain an initialized `Gravel_BumpAlloc` backed by `buf`. */
GravelBumpAlloc gravel_bump_alloc_init(GravelBuffer buf);

#endif
