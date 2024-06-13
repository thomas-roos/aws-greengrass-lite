/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gravel/client.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include <errno.h>

int main(void) {
    GravelBuffer iotcored = GRAVEL_STR("/aws/gravel/iotcored");

    GravelConn *conn;
    int ret = gravel_connect(iotcored, &conn);
    if (ret != 0) {
        GRAVEL_LOGE(
            "mqtt-client",
            "Failed to connect to %.*s",
            (int) iotcored.len,
            iotcored.data
        );
        return EHOSTUNREACH;
    }

    GravelList args = GRAVEL_LIST(GRAVEL_OBJ_MAP(
        { GRAVEL_STR("topic"), GRAVEL_OBJ_STR("hello") },
        { GRAVEL_STR("payload"), GRAVEL_OBJ_STR("hello world") },
    ));

    gravel_notify(conn, GRAVEL_STR("publish"), args);

    GRAVEL_LOGI("mqtt-client", "Sent MQTT publish.");
}
