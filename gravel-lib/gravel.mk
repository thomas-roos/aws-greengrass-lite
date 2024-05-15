# gravel - Utilities for AWS IoT Core clients
# Copyright (C) 2024 Amazon.com, Inc. or its affiliates

GRAVEL_LOG_LEVEL ?= GRAVEL_LOG_INFO
CPPFLAGS += -DGRAVEL_LOG_LEVEL=$(GRAVEL_LOG_LEVEL)
