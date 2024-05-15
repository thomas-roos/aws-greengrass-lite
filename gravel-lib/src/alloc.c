/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/alloc.h"
#include "gravel/log.h"
#include <assert.h>

void *gravel_alloc(GravelAlloc *alloc, size_t size, size_t alignment) {
    assert((alloc != NULL) && (alloc->alloc != NULL));

    void *ret = alloc->alloc(alloc, size, alignment);

    if (ret == NULL) {
        GRAVEL_LOGW(
            "gravel-lib", "[%p] Failed alloc %zu bytes.", (void *) alloc, size
        );
    } else {
        GRAVEL_LOGT(
            "gravel-lib", "[%p] alloc %p, len %zu.", (void *) alloc, ret, size
        );
    }

    return ret;
}

void gravel_free(GravelAlloc *alloc, void *ptr) {
    assert(alloc != NULL);

    GRAVEL_LOGT("gravel-lib", "[%p] Free %p", (void *) alloc, ptr);

    if ((alloc->free != NULL) && (ptr != NULL)) {
        alloc->free(alloc, ptr);
    }
}
