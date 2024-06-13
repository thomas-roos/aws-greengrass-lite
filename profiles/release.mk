# gravel - Utilities for AWS IoT Core clients
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

CPPFLAGS += -DNDEBUG
CFLAGS += -O3 -flto -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections,-s
