/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GRAVEL_MSGPACK_H
#define GRAVEL_MSGPACK_H

#include "gravel/alloc.h"
#include "gravel/object.h"

/** Maximum size of msgpack payload to read/write.
 * Can be configured with `-DGRAVEL_MSGPACK_MAX_MSG_LEN=<N>`. */
#ifndef GRAVEL_MSGPACK_MAX_MSG_LEN
#define GRAVEL_MSGPACK_MAX_MSG_LEN 10000
#endif

/** Writes object to a buffer in msgpack encoding.
 * `buf` must be initialized to the buffer to write to. Its length will be
 * updated to the length of the encoded data. */
int gravel_msgpack_encode(GravelObject obj, GravelBuffer *buf);

/** Decodes msgpack data into an object.
 * All returned data is allocated with `alloc`, and does not reference `buf`. */
int gravel_msgpack_decode(
    GravelAlloc *alloc, GravelBuffer buf, GravelObject *obj
);

/** Partially decodes msgpack data, and updates `buf` to remaining data.
 * Returned buffers point into `buf`.
 * Returned Lists and Maps have NULL for elements, and should only be used for
 * length. Reading is stopped before any List/Map contained elements. */
int gravel_msgpack_decode_lazy_noalloc(GravelBuffer *buf, GravelObject *obj);

#endif
