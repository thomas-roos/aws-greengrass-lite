/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_DEFER_H
#define GRAVEL_DEFER_H

/*! Macros for automatic resource cleanup */

#include "alloc.h"
#include "macro_util.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

// clang-format off
/** Run clean up for `fn` when this falls out of scope. */
#define GRAVEL_DEFER(fn, ...) \
    __attribute__((cleanup(gravel_defer__##fn))) \
    GravelDeferRecord__##fn \
    GRAVEL_MACRO_PASTE(\
        gravel_defer_record__, GRAVEL_MACRO_FIRST(__VA_ARGS__, ) \
    ) = { &GRAVEL_MACRO_FIRST(__VA_ARGS__, ), \
          GRAVEL_MACRO_REST(__VA_ARGS__, ) }; \
    static void (* const GRAVEL_MACRO_PASTE( \
        gravel_defer_fn__, GRAVEL_MACRO_FIRST(__VA_ARGS__, ) \
    ))(GravelDeferRecord__##fn *) __attribute__((unused)) = \
        gravel_defer__##fn; \
    static void (* const GRAVEL_MACRO_PASTE( \
        gravel_defer_cancel_fn__, GRAVEL_MACRO_FIRST(__VA_ARGS__, ) \
    ))(GravelDeferRecord__##fn *) __attribute__((unused)) = \
        gravel_cancel_defer__##fn
// clang-format on

/** Cancel cleanup for `id`. */
#define GRAVEL_DEFER_CANCEL(id) \
    gravel_defer_cancel_fn__##id(&gravel_defer_record__##id)

/** Immediately cleanup `id`. */
#define GRAVEL_DEFER_FORCE(id) \
    gravel_defer_fn__##id(&gravel_defer_record__##id); \
    GRAVEL_DEFER_CANCEL(id)

/** Enable defer for a function with one parameter. */
#define GRAVEL_DEFINE_DEFER(fn, type, name, cleanup) \
    typedef type *GravelDeferRecord__##fn; \
    static inline void gravel_defer__##fn(type **defer_record_ptr) { \
        type *name = *defer_record_ptr; \
        if (name != NULL) { \
            cleanup; \
        } \
    } \
    static inline void gravel_cancel_defer__##fn(type **defer_record_ptr) { \
        *defer_record_ptr = NULL; \
    }

// Common deferred functions

/** Enable defer for closing file descriptors. */
GRAVEL_DEFINE_DEFER(close, int, fd, if (*fd >= 0) close(*fd))

/** Enable defer for freeing system allocated pointers. */
GRAVEL_DEFINE_DEFER(free, void *, p, free(*p))

/** Enable defer for unlocking mutexes. */
GRAVEL_DEFINE_DEFER(
    pthread_mutex_unlock, pthread_mutex_t, mut, pthread_mutex_unlock(mut)
)

/** Enable defer for `gravel_free` */

typedef struct {
    void **ptr;
    GravelAlloc *ctx;
} GravelDeferRecord__gravel_free;

static inline void
gravel_defer__gravel_free(GravelDeferRecord__gravel_free *record) {
    gravel_free(record->ctx, *record->ptr);
}

static inline void
gravel_cancel_defer__gravel_free(GravelDeferRecord__gravel_free *record) {
    *record->ptr = NULL;
}

#endif
