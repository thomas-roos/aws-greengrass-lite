# gravel - Utilities for AWS IoT Core clients
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

CFLAGS += -Og -ggdb3 -ftrivial-auto-var-init=pattern -fno-omit-frame-pointer \
          -fsanitize=undefined
LDFLAGS += -Wl,--compress-debug-sections=zlib
