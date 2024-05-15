# gravel - Utilities for AWS IoT Core clients
# Copyright (C) 2024 Amazon.com, Inc. or its affiliates

CFLAGS += -Og -ggdb3 -ftrivial-auto-var-init=pattern -fno-omit-frame-pointer \
          -fsanitize=undefined
LDFLAGS += -Wl,--compress-debug-sections=zlib
