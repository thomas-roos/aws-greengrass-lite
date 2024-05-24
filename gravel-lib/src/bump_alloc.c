/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/bump_alloc.h"
#include "gravel/alloc.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include <stdlib.h>

static void *
bump_alloc_alloc(GravelAlloc *alloc, size_t size, size_t alignment) {
    GravelBumpAlloc *ctx = (GravelBumpAlloc *) alloc;

    size_t pad = (alignment - (ctx->index % alignment)) % alignment;
    size_t idx = ctx->index + pad;

    if (pad > 0) {
        GRAVEL_LOGD(
            "gravel-lib", "[%p] Need %zu padding.", (void *) alloc, pad
        );
    }

    if (idx + size >= ctx->buf.len) {
        return NULL;
    }

    ctx->index = idx + size;
    return &ctx->buf.data[idx];
}

GravelBumpAlloc gravel_bump_alloc_init(GravelBuffer buf) {
    return (GravelBumpAlloc) {
        .alloc = { .ALLOC = bump_alloc_alloc, .FREE = NULL },
        .buf = buf,
        .index = 0,
    };
}
