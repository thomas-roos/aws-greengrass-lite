/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef IOTCORED_ARGS_H
#define IOTCORED_ARGS_H

typedef struct {
    char *endpoint;
    char *id;
    char *rootca;
    char *cert;
    char *key;
} IotcoredArgs;

#endif
