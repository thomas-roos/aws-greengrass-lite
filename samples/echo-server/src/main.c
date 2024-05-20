/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/buffer.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include "gravel/server.h"
#include <errno.h>

void gravel_receive_callback(
    void *ctx,
    GravelBuffer method,
    GravelList params,
    GravelResponseHandle *handle
) {
    (void) ctx;

    if ((params.len < 1) && (params.items[0].type != GRAVEL_TYPE_MAP)) {
        GRAVEL_LOGE("rpc-handler", "Publish received invalid arguments.");
        gravel_respond(handle, EINVAL, GRAVEL_OBJ_NULL());
        return;
    }

    GravelMap param_map = params.items[0].map;

    if (gravel_buffer_eq(method, GRAVEL_STR("echo"))) {
        gravel_respond(handle, 0, GRAVEL_OBJ(param_map));
        return;
    }

    gravel_respond(handle, EINVAL, GRAVEL_OBJ_NULL());
}

int main(void) {
    gravel_listen(GRAVEL_STR("/aws/gravel/echo-server"), NULL);
}
