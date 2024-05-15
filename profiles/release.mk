# gravel - Utilities for AWS IoT Core clients
# Copyright (C) 2024 Amazon.com, Inc. or its affiliates

CPPFLAGS += -DNDEBUG
CFLAGS += -O3 -flto -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections,-s
