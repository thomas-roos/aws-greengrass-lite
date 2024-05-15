/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/server.h"
#include "gravel/bump_alloc.h"
#include "gravel/defer.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include "msgpack.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

/** Maximum number of outstanding responses.
 * Can be configured with `-DGRAVEL_SERVER_MAX_OUTSTANDING=<N>`. */
#ifndef GRAVEL_SERVER_MAX_OUTSTANDING
#define GRAVEL_SERVER_MAX_OUTSTANDING 1
#endif

/** Buffer length for storing objects from decoding msgpack payloads.
 * Can be configured with `-DGRAVEL_MSGPACK_DECODE_BUFFER_LEN=<N>`. */
#ifndef GRAVEL_MSGPACK_DECODE_BUFFER_LEN
#define GRAVEL_MSGPACK_DECODE_BUFFER_LEN (50 * sizeof(GravelObject))
#endif

static pthread_mutex_t payload_array_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8_t payload_array[GRAVEL_MSGPACK_MAX_MSG_LEN];
static uint8_t decode_array[GRAVEL_MSGPACK_DECODE_BUFFER_LEN];

static pthread_mutex_t encode_array_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8_t encode_array[GRAVEL_MSGPACK_MAX_MSG_LEN];

struct GravelResponseHandle {
    int respfd;
    uint32_t msgid;
};

static GravelResponseHandle handles[GRAVEL_SERVER_MAX_OUTSTANDING];
pthread_mutex_t handles_mutex = PTHREAD_MUTEX_INITIALIZER;

static const int HANDLE_FREE = -2;
static const int HANDLE_UNINIT = -3;

__attribute__((constructor)) static void init_handles(void) {
    for (size_t i = 0; i < GRAVEL_SERVER_MAX_OUTSTANDING; i++) {
        handles[i] = (GravelResponseHandle) { .respfd = HANDLE_FREE };
    }
}

GRAVEL_DEFINE_DEFER(
    release_handle,
    GravelResponseHandle *,
    handle,
    (*handle)->respfd = HANDLE_FREE
)

static GravelResponseHandle *get_free_handle(void) {
    while (true) {
        {
            pthread_mutex_lock(&handles_mutex);
            GRAVEL_DEFER(pthread_mutex_unlock, handles_mutex);

            for (size_t i = 0; i < GRAVEL_SERVER_MAX_OUTSTANDING; i++) {
                if (handles[i].respfd == HANDLE_FREE) {
                    handles[i] = (GravelResponseHandle) {
                        .respfd = HANDLE_UNINIT,
                        .msgid = 0,
                    };
                    return &handles[i];
                }
            }
        }
        sleep(1);
    }
}

static int parse_incoming(
    GravelBuffer buf,
    bool *needs_resp,
    uint32_t *msgid,
    GravelBuffer *method,
    GravelBuffer *params
) {
    assert(
        (needs_resp != NULL) && (msgid != NULL) && (method != NULL)
        && (params != NULL)
    );

    GravelBuffer msg = buf;
    GravelObject obj;

    int ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
    if (ret != 0) return ret;

    if ((obj.type != GRAVEL_TYPE_LIST)) {
        GRAVEL_LOGE("msgpack-rpc", "Received payload that is not an array.");
        return EPROTO;
    }

    size_t mpk_len = obj.list.len;

    if (mpk_len < 3) {
        GRAVEL_LOGE("msgpack-rpc", "Received payload that is too small array.");
        return EPROTO;
    }

    // type
    ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
    if (ret != 0) return ret;

    if (obj.type != GRAVEL_TYPE_I64) {
        GRAVEL_LOGE("msgpack-rpc", "Received payload type invalid.");
        return EPROTO;
    }

    int64_t type = obj.i64;

    if (type == 0) {
        // request
        if (mpk_len != 4) {
            GRAVEL_LOGE("msgpack-rpc", "Received payload not 4 element array.");
            return EPROTO;
        }

        // msgid
        ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
        if (ret != 0) return ret;

        if ((obj.type != GRAVEL_TYPE_U64) || (obj.u64 > UINT32_MAX)) {
            GRAVEL_LOGE("msgpack-rpc", "Received payload msgid invalid.");
            return EPROTO;
        }

        *msgid = (uint32_t) obj.u64;
        *needs_resp = true;
    } else if (type == 2) {
        // notification
        if (mpk_len != 3) {
            GRAVEL_LOGE("msgpack-rpc", "Received payload not 3 element array.");
            return EPROTO;
        }
        *msgid = 0;
        *needs_resp = false;
    } else {
        GRAVEL_LOGE(
            "msgpack-rpc",
            "Received payload type invalid: %lu",
            (long unsigned) type
        );
        return EPROTO;
    }

    // method
    ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
    if (ret != 0) return ret;

    if (obj.type != GRAVEL_TYPE_BUF) {
        GRAVEL_LOGE("msgpack-rpc", "Received non-raw method.");
        return EPROTO;
    }

    *method = obj.buf;

    // params
    GravelBuffer copy = msg;
    ret = gravel_msgpack_decode_lazy_noalloc(&copy, &obj);
    if (ret != 0) return ret;

    if (obj.type != GRAVEL_TYPE_LIST) {
        GRAVEL_LOGE("msgpack-rpc", "Received non-array params.");
        return EPROTO;
    }

    *params = msg;

    return 0;
}

noreturn void
gravel_listen(GravelBuffer path, GravelReceiveCallback callback, void *ctx) {
    while (true) {
        int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        if (sockfd == -1) {
            int err = errno;
            GRAVEL_LOGE(
                "msgpack-rpc", "Failed to create socket: %s", strerror(err)
            );
            sleep(5);
            continue;
        }
        GRAVEL_DEFER(close, sockfd);

        struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = { 0 } };

        // Skipping first byte (makes socket in abstract namespace)

        size_t copy_len = path.len <= sizeof(addr.sun_path) - 1
            ? path.len
            : sizeof(addr.sun_path) - 1;

        if (copy_len < path.len) {
            GRAVEL_LOGW(
                "msgpack-rpc",
                "Truncating path to %u bytes [%.*s]",
                (unsigned) copy_len,
                (int) path.len,
                (char *) path.data
            );
        }

        memcpy(&addr.sun_path[1], path.data, copy_len);

        if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            int err = errno;
            GRAVEL_LOGE(
                "msgpack-rpc", "Failed to bind socket: %s", strerror(err)
            );
            sleep(5);
            continue;
        }

        if (listen(sockfd, 20) == -1) {
            int err = errno;
            GRAVEL_LOGE(
                "msgpack-rpc", "Failed to listen socket: %s", strerror(err)
            );
            sleep(5);
            continue;
        }

        while (true) {
            int clientfd = accept(sockfd, NULL, NULL);
            if (clientfd == -1) {
                int err = errno;
                GRAVEL_LOGE(
                    "msgpack-rpc",
                    "Failed to accept on socket: %s",
                    strerror(err)
                );
                break;
            }
            GRAVEL_DEFER(close, clientfd);

            while (true) {
                pthread_mutex_lock(&payload_array_mtx);
                GRAVEL_DEFER(pthread_mutex_unlock, payload_array_mtx);

                GravelBuffer recv_buffer = GRAVEL_BUF(payload_array);

                ssize_t sys_ret = recv(
                    clientfd, recv_buffer.data, recv_buffer.len, MSG_TRUNC
                );
                if (sys_ret < 0) {
                    int err = errno;
                    GRAVEL_LOGE(
                        "msgpack-rpc",
                        "Failed to recv from client: %s",
                        strerror(err)
                    );
                    break;
                }

                recv_buffer.len = (size_t) sys_ret;

                if ((size_t) sys_ret > recv_buffer.len) {
                    // TODO: respond with server error if request
                    GRAVEL_LOGE(
                        "msgpack-rpc",
                        "Payload too large: size %zu, max %zu",
                        (size_t) sys_ret,
                        recv_buffer.len
                    );
                    continue;
                }

                if ((size_t) sys_ret == 0) {
                    GRAVEL_LOGI("msgpack-rpc", "Connection closed.");
                    break;
                }

                bool needs_resp;
                uint32_t msgid;
                GravelBuffer method;
                GravelBuffer params_buf;

                int ret = parse_incoming(
                    recv_buffer, &needs_resp, &msgid, &method, &params_buf
                );
                if (ret != 0) break;

                GravelBumpAlloc decode_mem
                    = gravel_bump_alloc_init(GRAVEL_BUF(decode_array));
                GravelObject params_obj;

                ret = gravel_msgpack_decode(
                    &decode_mem.alloc, params_buf, &params_obj
                );
                if (ret != 0) {
                    GRAVEL_LOGE(
                        "msgpack-rpc", "Failed decoding incoming payload."
                    );
                    // TODO: Send server error
                    break;
                };

                if (params_obj.type != GRAVEL_TYPE_LIST) {
                    GRAVEL_LOGE(
                        "msgpack-rpc", "Incoming payload params not list."
                    );
                    // TODO: Send server error
                    break;
                }

                GravelList params = params_obj.list;

                GravelResponseHandle *handle = NULL;

                if (needs_resp) {
                    handle = get_free_handle();
                    *handle = (GravelResponseHandle) {
                        .msgid = msgid,
                        .respfd = clientfd,
                    };
                }

                callback(ctx, method, params, handle);
            }
        }
    }
}

void gravel_respond(
    GravelResponseHandle *handle, int error, GravelObject value
) {
    if (handle == NULL) return;

    GRAVEL_DEFER(release_handle, handle);

    GravelObject payload = GRAVEL_OBJ_LIST(
        GRAVEL_OBJ_I64(1),
        GRAVEL_OBJ_U64(handle->msgid),
        (error != 0) ? GRAVEL_OBJ_I64(error) : GRAVEL_OBJ_NULL(),
        (error != 0) ? GRAVEL_OBJ_NULL() : value
    );

    pthread_mutex_lock(&encode_array_mtx);
    GRAVEL_DEFER(pthread_mutex_unlock, encode_array_mtx);

    GravelBuffer encoded = GRAVEL_BUF(encode_array);

    int ret = gravel_msgpack_encode(payload, &encoded);
    if (ret != 0) {
        GRAVEL_LOGE("msgpack-rpc", "Failed to encode response.");
        return;
    }

    ssize_t result = send(handle->respfd, encoded.data, encoded.len, 0);
    if (result <= 0) {
        int err = errno;
        GRAVEL_LOGE(
            "msgpack-rpc", "Failed to send response: %s", strerror(err)
        );
    }
}
