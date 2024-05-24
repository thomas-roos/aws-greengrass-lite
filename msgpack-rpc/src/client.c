/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "gravel/client.h"
#include "gravel/alloc.h"
#include "gravel/defer.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include "gravel/utils.h"
#include "msgpack.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/** Maximum number of simultaneous client connections.
 * Can be configured with `-DGRAVEL_CLIENT_CONN_MAX=<N>`. */
#ifndef GRAVEL_CLIENT_CONN_MAX
#define GRAVEL_CLIENT_CONN_MAX 1
#endif

static pthread_mutex_t payload_array_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8_t payload_array[GRAVEL_MSGPACK_MAX_MSG_LEN];

struct GravelConn {
    int sockfd;
    uint32_t counter;
};

static GravelConn conns[GRAVEL_CLIENT_CONN_MAX];
pthread_mutex_t conns_mutex = PTHREAD_MUTEX_INITIALIZER;

static const int CONNS_FREE = -2;
static const int CONNS_UNINIT = -3;

__attribute__((constructor)) static void init_conns(void) {
    for (size_t i = 0; i < GRAVEL_CLIENT_CONN_MAX; i++) {
        conns[i] = (GravelConn) { .sockfd = CONNS_FREE };
    }
}

static GravelConn *get_free_conn(void) {
    pthread_mutex_lock(&conns_mutex);
    GRAVEL_DEFER(pthread_mutex_unlock, conns_mutex);

    for (size_t i = 0; i < GRAVEL_CLIENT_CONN_MAX; i++) {
        if (conns[i].sockfd == CONNS_FREE) {
            conns[i] = (GravelConn) {
                .sockfd = CONNS_UNINIT,
                .counter = 0,
            };
            return &conns[i];
        }
    }
    return NULL;
}

__attribute__((weak)) int gravel_connect(GravelBuffer path, GravelConn **conn) {
    assert(conn != NULL);

    *conn = NULL;

    int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sockfd == -1) {
        int err = errno;
        GRAVEL_LOGE("msgpack-rpc", "Failed to create socket: %d.", err);
        return err;
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

    if (connect(sockfd, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        int err = errno;
        GRAVEL_LOGW("msgpack-rpc", "Failed to connect to server: %d.", err);
        return err;
    }

    GravelConn *c = get_free_conn();
    if (c == NULL) {
        return ENOBUFS;
    }

    GRAVEL_DEFER_CANCEL(sockfd);
    *c = (GravelConn) { .sockfd = sockfd, .counter = 0 };
    *conn = c;
    return 0;
}

void gravel_close(GravelConn *conn) {
    assert(conn != NULL);

    pthread_mutex_lock(&conns_mutex);
    GRAVEL_DEFER(pthread_mutex_unlock, conns_mutex);

    close(conn->sockfd);
    conn->sockfd = CONNS_FREE;
}

static int parse_incoming(
    GravelBuffer buf, uint32_t *msgid, bool *error, GravelBuffer *value
) {
    GravelBuffer msg = buf;
    GravelObject obj;

    int ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
    if (ret != 0) {
        return ret;
    }

    if ((obj.type != GRAVEL_TYPE_LIST) || (obj.list.len != 4)) {
        GRAVEL_LOGE("msgpack-rpc", "Received payload not 4 element array.");
        return EPROTO;
    }

    // payload type
    ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
    if (ret != 0) {
        return ret;
    }

    if ((obj.type != GRAVEL_TYPE_I64) || (obj.i64 != 1)) {
        GRAVEL_LOGE("msgpack-rpc", "Received payload type invalid.");
        return EPROTO;
    }

    // msgid
    ret = gravel_msgpack_decode_lazy_noalloc(&msg, &obj);
    if (ret != 0) {
        return ret;
    }

    if ((obj.type != GRAVEL_TYPE_U64) || (obj.u64 > UINT32_MAX)) {
        GRAVEL_LOGE("msgpack-rpc", "Received payload msgid invalid.");
        return EPROTO;
    }

    *msgid = (uint32_t) obj.u64;

    // error
    GravelBuffer copy = msg;
    ret = gravel_msgpack_decode_lazy_noalloc(&copy, &obj);
    if (ret != 0) {
        return ret;
    }

    if (obj.type != GRAVEL_TYPE_NULL) {
        msg.len -= 1; // Account for result (should be nil, aka 1 byte)
        *error = true;
        *value = msg;
        return 0;
    }

    // result
    *error = false;
    *value = copy;
    return 0;
}

int gravel_call(
    GravelConn *conn,
    GravelBuffer method,
    GravelList params,
    GravelAlloc *alloc,
    GravelObject *result
) {
    assert(conn != NULL);

    uint32_t msgid;
    msgid = conn->counter++;

    GravelObject payload = GRAVEL_OBJ_LIST(
        GRAVEL_OBJ_I64(0),
        GRAVEL_OBJ_U64(msgid),
        GRAVEL_OBJ(method),
        GRAVEL_OBJ(params)
    );

    ssize_t sys_ret;

    {
        pthread_mutex_lock(&payload_array_mtx);
        GRAVEL_DEFER(pthread_mutex_unlock, payload_array_mtx);

        GravelBuffer send_buffer = GRAVEL_BUF(payload_array);

        int ret = gravel_msgpack_encode(payload, &send_buffer);
        if (ret != 0) {
            return ret;
        }

        sys_ret = send(conn->sockfd, send_buffer.data, send_buffer.len, 0);
    }

    if (sys_ret < 0) {
        int err = errno;
        GRAVEL_LOGE("msgpack-rpc", "Failed to send: %d.", err);
        return err;
    }

    while (true) {
        pthread_mutex_lock(&payload_array_mtx);
        GRAVEL_DEFER(pthread_mutex_unlock, payload_array_mtx);

        GravelBuffer recv_buffer = GRAVEL_BUF(payload_array);

        sys_ret = recv(
            conn->sockfd,
            recv_buffer.data,
            recv_buffer.len,
            MSG_PEEK | MSG_TRUNC
        );

        if (sys_ret < 0) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            GRAVEL_LOGE("msgpack-rpc", "Failed recv: %d.", err);
            return err;
        }

        if ((size_t) sys_ret > recv_buffer.len) {
            GRAVEL_LOGE(
                "msgpack-rpc",
                "Payload too large: size %zu, max %zu",
                (size_t) sys_ret,
                recv_buffer.len
            );
            return EMSGSIZE;
        }

        recv_buffer.len = (size_t) sys_ret;

        // Must not alloc here, as we want to claim message even if ENOMEM
        uint32_t ret_id;
        bool error;
        GravelBuffer result_buf;
        int ret = parse_incoming(recv_buffer, &ret_id, &error, &result_buf);
        if (ret != 0) {
            return ret;
        }

        if (ret_id != msgid) {
            // not ours
            GRAVEL_DEFER_FORCE(payload_array_mtx);
            gravel_sleep(1);
            continue;
        }

        // claim message
        (void) recv(conn->sockfd, NULL, 0, MSG_TRUNC);

        ret = gravel_msgpack_decode(alloc, result_buf, result);
        if (ret != 0) {
            GRAVEL_LOGE("msgpack-rpc", "Failed to decode payload response.");
            return EPROTO;
        }

        return 0;
    }
}

int gravel_notify(GravelConn *conn, GravelBuffer method, GravelList params) {
    assert(conn != NULL);

    GravelObject payload = GRAVEL_OBJ_LIST(
        GRAVEL_OBJ_I64(2), GRAVEL_OBJ(method), GRAVEL_OBJ(params)
    );

    ssize_t sys_ret;

    {
        pthread_mutex_lock(&payload_array_mtx);
        GRAVEL_DEFER(pthread_mutex_unlock, payload_array_mtx);

        GravelBuffer send_buffer = GRAVEL_BUF(payload_array);

        int ret = gravel_msgpack_encode(payload, &send_buffer);
        if (ret != 0) {
            return ret;
        }

        sys_ret = send(conn->sockfd, send_buffer.data, send_buffer.len, 0);
    }

    if (sys_ret < 0) {
        int err = errno;
        GRAVEL_LOGE("msgpack-rpc", "Failed to send: %d.", err);
        return err;
    }

    return 0;
}
