/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/bump_alloc.h"
#include "gravel/client.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include <errno.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>

int main(void) {
    GravelBuffer server = GRAVEL_STR("/aws/gravel/echo-server");
    static uint8_t buffer[10 * sizeof(GravelObject)] = { 0 };

    GravelConn *conn;
    int ret = gravel_connect(server, &conn);
    if (ret != 0) {
        GRAVEL_LOGE(
            "client", "Failed to connect to %.*s", (int) server.len, server.data
        );
        return EHOSTUNREACH;
    }

    GravelList args
        = GRAVEL_LIST(GRAVEL_OBJ_STR("hello"), GRAVEL_OBJ_STR("world"));

    struct timespec before;
    struct timespec after;
    clock_gettime(CLOCK_REALTIME, &before);

    for (size_t i = 0; i < 1000000; i++) {
        GravelBumpAlloc alloc = gravel_bump_alloc_init(GRAVEL_BUF(buffer));
        GravelObject result;

        ret = gravel_call(
            conn, GRAVEL_STR("publish"), args, &alloc.alloc, &result
        );

        if (ret != 0) {
            GRAVEL_LOGE("client", "Failed to send publish: %d.", ret);
            break;
        }
    }

    clock_gettime(CLOCK_REALTIME, &after);
    double elapsed_nsecs = (double) (after.tv_sec - before.tv_sec)
        + (double) (after.tv_nsec - before.tv_nsec) / 1000000000;

    GRAVEL_LOGE("client", "Time: %f", elapsed_nsecs);
}
