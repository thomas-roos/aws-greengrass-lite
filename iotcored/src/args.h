/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
