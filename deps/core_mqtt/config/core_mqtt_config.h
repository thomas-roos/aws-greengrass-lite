/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

#include "core_mqtt.h"
#include <pthread.h>

pthread_mutex_t *coremqtt_get_send_mtx(const MQTTContext_t *ctx);
pthread_mutex_t *coremqtt_get_state_mtx(const MQTTContext_t *ctx);

#define MQTT_RECV_POLLING_TIMEOUT_MS (1000)

#ifdef CORE_MQTT_SOURCE

#include "gravel/log.h"

#define GRAVEL_MQTT_LOGUNPACK(...) __VA_ARGS__
#define GRAVEL_MQTT_LOG2(fn, ...) fn("coreMQTT", __VA_ARGS__)
#define GRAVEL_MQTT_LOG(fn, ...) GRAVEL_MQTT_LOG2(fn, __VA_ARGS__)

#define LogDebug(body) GRAVEL_MQTT_LOG(GRAVEL_LOGD, GRAVEL_MQTT_LOGUNPACK body)
#define LogInfo(body) GRAVEL_MQTT_LOG(GRAVEL_LOGI, GRAVEL_MQTT_LOGUNPACK body)
#define LogWarn(body) GRAVEL_MQTT_LOG(GRAVEL_LOGW, GRAVEL_MQTT_LOGUNPACK body)
#define LogError(body) GRAVEL_MQTT_LOG(GRAVEL_LOGE, GRAVEL_MQTT_LOGUNPACK body)

#define MQTT_PRE_SEND_HOOK(pContext) \
    pthread_mutex_lock(coremqtt_get_send_mtx(pContext));
#define MQTT_POST_SEND_HOOK(pContext) \
    pthread_mutex_unlock(coremqtt_get_send_mtx(pContext));
#define MQTT_PRE_STATE_UPDATE_HOOK(pContext) \
    pthread_mutex_lock(coremqtt_get_state_mtx(pContext));
#define MQTT_POST_STATE_UPDATE_HOOK(pContext) \
    pthread_mutex_unlock(coremqtt_get_state_mtx(pContext));

#endif

#endif
