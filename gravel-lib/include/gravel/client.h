/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_COMMS_CLIENT_H
#define GRAVEL_COMMS_CLIENT_H

/*! Pluggable RPC client interface */

#include "alloc.h"
#include "object.h"

typedef struct GravelConn GravelConn;

/** Open a connection to server on `path`. */
int gravel_connect(GravelBuffer path, GravelConn **conn)
    __attribute__((warn_unused_result));

/** Close a connection to a server. */
void gravel_close(GravelConn *conn);

/** Make an RPC call.
 * `result` will use memory from `alloc` if needed. */
int gravel_call(
    GravelConn *conn,
    GravelBuffer method,
    GravelList params,
    GravelAlloc *alloc,
    GravelObject *result
) __attribute__((warn_unused_result));

/** Make an RPC notification (no response). */
int gravel_notify(GravelConn *conn, GravelBuffer method, GravelList params);

#endif
