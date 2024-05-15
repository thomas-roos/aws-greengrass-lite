# gravel - Utilities for AWS IoT Core clients
# Copyright (C) 2024 Amazon.com, Inc. or its affiliates

deps/core_mqtt_INCDIRS := config \
        coreMQTT/source/include \
        coreMQTT/source/interface
deps/core_mqtt_SRCDIR := coreMQTT/source
deps/core_mqtt_CPPFLAGS := -DCORE_MQTT_SOURCE
deps/core_mqtt_LIBS := gravel-lib
