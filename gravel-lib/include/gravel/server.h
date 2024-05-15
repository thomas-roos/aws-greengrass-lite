/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_COMMS_SERVER_H
#define GRAVEL_COMMS_SERVER_H

#include "object.h"
#include <stdnoreturn.h>

/*! Pluggable RPC client interface */

typedef struct GravelResponseHandle GravelResponseHandle;

/** Callback that receives messages from listen.
 * `handle` will be non-NULL if client is expecting a response.
 * If `handle` is non-NULL, it must be passed to `gravel_respond` at some point.
 */
typedef void (*GravelReceiveCallback)(
    void *ctx,
    GravelBuffer method,
    GravelList params,
    GravelResponseHandle *handle
);

/** Listen on `path` and receive incoming RPC calls/notifications. */
noreturn void
gravel_listen(GravelBuffer path, GravelReceiveCallback callback, void *ctx);

/** Respond to a message received from listen.
 * Pass non-zero error to return an error, else value is the successful result.
 * On error the RPC protocol will send the error code if supported. It may use
 * the value as extra debugging information for the error.
 * If handle is NULL, does nothing. */
void gravel_respond(
    GravelResponseHandle *handle, int error, GravelObject value
);

#endif