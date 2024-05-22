/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef IOTCORED_TLS_H
#define IOTCORED_TLS_H

#include "args.h"
#include "gravel/object.h"

typedef struct IotcoredTlsCtx IotcoredTlsCtx;

int iotcored_tls_connect(const IotcoredArgs *args, IotcoredTlsCtx **ctx);

int iotcored_tls_read(IotcoredTlsCtx *ctx, GravelBuffer *buf);
int iotcored_tls_write(IotcoredTlsCtx *ctx, GravelBuffer buf);

void iotcored_tls_cleanup(IotcoredTlsCtx *ctx);

#endif
