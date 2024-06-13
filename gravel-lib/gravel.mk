# gravel - Utilities for AWS IoT Core clients
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

GRAVEL_LOG_LEVEL ?= INFO
CPPFLAGS += -DGRAVEL_LOG_LEVEL=GRAVEL_LOG_$(GRAVEL_LOG_LEVEL)
