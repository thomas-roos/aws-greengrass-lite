/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef IOTCORED_MQTT_H
#define IOTCORED_MQTT_H

#include "args.h"
#include "gravel/object.h"
#include <stdint.h>

typedef struct {
    GravelBuffer topic;
    GravelBuffer payload;
} IotcoredMsg;

int iotcored_mqtt_connect(const IotcoredArgs *args);

int iotcored_mqtt_publish(const IotcoredMsg *msg, uint8_t qos);
int iotcored_mqtt_subscribe(GravelBuffer topic_filter, uint8_t qos);

#endif
