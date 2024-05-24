/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#include "./msgpack.h"
#include "gravel/alloc.h"
#include "gravel/log.h"
#include "gravel/object.h"
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static_assert((uint32_t) -1 == 0xFFFFFFFFUL, "twos-compliment required");

static int decode_obj(
    bool noalloc, GravelAlloc *alloc, GravelBuffer *buf, GravelObject *obj
);

static int
buf_split(GravelBuffer buf, GravelBuffer *left, GravelBuffer *right, size_t n) {
    if (n > buf.len) {
        return EBADMSG;
    }

    if (left != NULL) {
        *left = (GravelBuffer) { .len = n, .data = buf.data };
    }
    if (right != NULL) {
        *right = (GravelBuffer) { .len = buf.len - n, .data = &buf.data[n] };
    }
    return 0;
}

static int read_uint(GravelBuffer *buf, size_t bytes, uint64_t *result) {
    assert((buf != NULL) && (result != NULL));
    assert((bytes <= sizeof(uint64_t)) && (bytes > 0));

    GravelBuffer read_from;
    int ret = buf_split(*buf, &read_from, buf, bytes);
    if (ret != 0) {
        return ret;
    }

    uint64_t value = 0;
    memcpy(&((char *) &value)[sizeof(uint64_t) - bytes], read_from.data, bytes);
    value = be64toh(value);

    *result = value;
    return 0;
}

static int decode_uint(GravelBuffer *buf, size_t bytes, GravelObject *obj) {
    assert((buf != NULL) && (obj != NULL));

    uint64_t value;

    int ret = read_uint(buf, bytes, &value);
    if (ret != 0) {
        return ret;
    }

    *obj = GRAVEL_OBJ_U64(value);
    return 0;
}

static int decode_int(GravelBuffer *buf, size_t bytes, GravelObject *obj) {
    assert((buf != NULL) && (obj != NULL));
    assert((bytes <= sizeof(uint64_t)) && (bytes > 0));

    GravelBuffer read_from;
    int ret = buf_split(*buf, &read_from, buf, bytes);
    if (ret != 0) {
        return ret;
    }

    bool negative = (read_from.data[0] & 0x80) > 0;

    // account for sign-extension
    uint64_t value_bytes = negative ? UINT64_MAX : 0;
    memcpy(
        &((char *) &value_bytes)[sizeof(uint64_t) - bytes],
        read_from.data,
        bytes
    );
    value_bytes = be64toh(value_bytes);

    int64_t value;
    memcpy(&value, &value_bytes, sizeof(uint64_t));

    *obj = GRAVEL_OBJ_I64(value);
    return 0;
}

static int decode_buf(
    bool noalloc,
    GravelAlloc *alloc,
    GravelBuffer *buf,
    size_t len,
    GravelObject *obj
) {
    assert((buf != NULL) && (obj != NULL));

    GravelBuffer old;
    int ret = buf_split(*buf, &old, buf, len);
    if (ret != 0) {
        return ret;
    }

    if (noalloc) {
        *obj = GRAVEL_OBJ(old);
    } else {
        uint8_t *new_storage = GRAVEL_ALLOCN(alloc, uint8_t, len);
        if (new_storage == NULL) {
            return ENOMEM;
        }

        GravelBuffer new = { .data = new_storage, .len = len };
        memcpy(new.data, old.data, len);
        *obj = GRAVEL_OBJ(new);
    }

    return 0;
}

static int decode_len_buf(
    bool noalloc,
    GravelAlloc *alloc,
    GravelBuffer *buf,
    size_t len_bytes,
    GravelObject *obj
) {
    assert((buf != NULL) && (obj != NULL));

    uint64_t len;
    int ret = read_uint(buf, len_bytes, &len);
    if (ret != 0) {
        return ret;
    }

    return decode_buf(noalloc, alloc, buf, len, obj);
}

static int decode_f32(GravelBuffer *buf, GravelObject *obj) {
    static_assert(sizeof(float) == sizeof(int32_t), "float is not 32 bits");
    assert((buf != NULL) && (obj != NULL));

    uint64_t value_bytes_64;
    int ret = read_uint(buf, 4, &value_bytes_64);
    if (ret != 0) {
        return ret;
    }

    uint32_t value_bytes = (uint32_t) value_bytes_64;
    float value;
    memcpy(&value, &value_bytes, 4);

    *obj = GRAVEL_OBJ_F64(value);
    return 0;
}

static int decode_f64(GravelBuffer *buf, GravelObject *obj) {
    static_assert(sizeof(double) == sizeof(int64_t), "double is not 64 bits");
    assert((buf != NULL) && (obj != NULL));

    uint64_t value_bytes;
    int ret = read_uint(buf, 8, &value_bytes);
    if (ret != 0) {
        return ret;
    }

    double value;
    memcpy(&value, &value_bytes, 8);

    *obj = GRAVEL_OBJ_F64(value);
    return 0;
}

// NOLINTNEXTLINE(misc-no-recursion)
static int decode_array(
    bool noalloc,
    GravelAlloc *alloc,
    GravelBuffer *buf,
    size_t len,
    GravelObject *obj
) {
    assert((alloc != NULL) && (buf != NULL) && (obj != NULL));

    if (noalloc) {
        *obj = GRAVEL_OBJ((GravelList) { .len = len, .items = NULL });
    } else {
        GravelObject *items = GRAVEL_ALLOCN(alloc, GravelObject, len);
        if (items == NULL) {
            return ENOMEM;
        }

        for (size_t i = 0; i < len; i++) {
            int ret = decode_obj(noalloc, alloc, buf, &items[i]);
            if (ret != 0) {
                return ret;
            }
        }

        *obj = GRAVEL_OBJ((GravelList) { .len = len, .items = items });
    }

    return 0;
}

// NOLINTNEXTLINE(misc-no-recursion)
static int decode_len_array(
    bool noalloc,
    GravelAlloc *alloc,
    GravelBuffer *buf,
    size_t len_bytes,
    GravelObject *obj
) {
    assert((alloc != NULL) && (buf != NULL) && (obj != NULL));

    uint64_t len;
    int ret = read_uint(buf, len_bytes, &len);
    if (ret != 0) {
        return ret;
    }

    return decode_array(noalloc, alloc, buf, len, obj);
}

// NOLINTNEXTLINE(misc-no-recursion)
static int decode_map(
    bool noalloc,
    GravelAlloc *alloc,
    GravelBuffer *buf,
    size_t len,
    GravelObject *obj
) {
    assert((alloc != NULL) && (buf != NULL) && (obj != NULL));

    if (noalloc) {
        *obj = GRAVEL_OBJ((GravelMap) { .len = len, .pairs = NULL });
    } else {
        GravelKV *pairs = GRAVEL_ALLOCN(alloc, GravelKV, len);
        if (pairs == NULL) {
            return ENOMEM;
        }

        for (size_t i = 0; i < len; i++) {
            GravelObject key;
            int ret = decode_obj(noalloc, alloc, buf, &key);
            if (ret != 0) {
                return ret;
            }

            if (key.type != GRAVEL_TYPE_BUF) {
                GRAVEL_LOGE("msgpack", "Map has unsupported key type.");
                return ENOTSUP;
            }
            pairs[i].key = key.buf;

            ret = decode_obj(noalloc, alloc, buf, &pairs[i].val);
            if (ret != 0) {
                return ret;
            }
        }

        *obj = GRAVEL_OBJ((GravelMap) { .len = len, .pairs = pairs });
    }
    return 0;
}

// NOLINTNEXTLINE(misc-no-recursion)
static int decode_len_map(
    bool noalloc,
    GravelAlloc *alloc,
    GravelBuffer *buf,
    size_t len_bytes,
    GravelObject *obj
) {
    assert((alloc != NULL) && (buf != NULL) && (obj != NULL));

    uint64_t len;
    int ret = read_uint(buf, len_bytes, &len);
    if (ret != 0) {
        return ret;
    }

    return decode_map(noalloc, alloc, buf, len, obj);
}

// NOLINTNEXTLINE(misc-no-recursion,readability-function-cognitive-complexity)
static int decode_obj(
    bool noalloc, GravelAlloc *alloc, GravelBuffer *buf, GravelObject *obj
) {
    assert((alloc != NULL) && (buf != NULL) && (obj != NULL));

    if (buf->len < 1) {
        return EBADMSG;
    }
    uint8_t tag = buf->data[0];
    (void) buf_split(*buf, NULL, buf, 1);

    if (tag <= 0x7F) {
        // positive fixint
        *obj = GRAVEL_OBJ_I64(tag);
        return 0;
    }
    if (tag < 0x8F) {
        // fixmap
        return decode_map(noalloc, alloc, buf, tag & 15, obj);
    }
    if (tag < 0x9F) {
        // fixarray
        return decode_array(noalloc, alloc, buf, tag & 15, obj);
    }
    if (tag < 0xBF) {
        // fixstr
        return decode_buf(noalloc, alloc, buf, tag & 31, obj);
    }
    if (tag == 0xC0) {
        // nil
        *obj = GRAVEL_OBJ_NULL();
        return 0;
    }
    if (tag == 0xC1) {
        // never used
        GRAVEL_LOGE("msgpack", "Payload has invalid 0xC1 type tag.");
        return EBADMSG;
    }
    if (tag == 0xC2) {
        // false
        *obj = GRAVEL_OBJ_BOOL(false);
        return 0;
    }
    if (tag == 0xC3) {
        // true
        *obj = GRAVEL_OBJ_BOOL(true);
        return 0;
    }
    if (tag == 0xC4) {
        // bin 8
        return decode_len_buf(noalloc, alloc, buf, 1, obj);
    }
    if (tag == 0xC5) {
        // bin 16
        return decode_len_buf(noalloc, alloc, buf, 2, obj);
    }
    if (tag == 0xC6) {
        // bin 32
        return decode_len_buf(noalloc, alloc, buf, 4, obj);
    }
    if (tag <= 0xC9) {
        // ext
        GRAVEL_LOGE("msgpack", "Payload has unsupported ext type.");
        return ENOTSUP;
    }
    if (tag == 0xCA) {
        // float 32
        return decode_f32(buf, obj);
    }
    if (tag == 0xCB) {
        // float 64
        return decode_f64(buf, obj);
    }
    if (tag == 0xCC) {
        // uint 8
        return decode_uint(buf, 1, obj);
    }
    if (tag == 0xCD) {
        // uint 16
        return decode_uint(buf, 2, obj);
    }
    if (tag == 0xCE) {
        // uint 32
        return decode_uint(buf, 4, obj);
    }
    if (tag == 0xCF) {
        // uint 64
        return decode_uint(buf, 8, obj);
    }
    if (tag == 0xD0) {
        // int 8
        return decode_int(buf, 1, obj);
    }
    if (tag == 0xD1) {
        // int 16
        return decode_int(buf, 2, obj);
    }
    if (tag == 0xD2) {
        // int 32
        return decode_int(buf, 4, obj);
    }
    if (tag == 0xD3) {
        // int 64
        return decode_int(buf, 8, obj);
    }
    if (tag <= 0xD8) {
        // fixext
        GRAVEL_LOGE("msgpack", "Payload has unsupported ext type.");
        return ENOTSUP;
    }
    if (tag == 0xD9) {
        // str 8
        return decode_len_buf(noalloc, alloc, buf, 1, obj);
    }
    if (tag == 0xDA) {
        // str 16
        return decode_len_buf(noalloc, alloc, buf, 2, obj);
    }
    if (tag == 0xDB) {
        // str 32
        return decode_len_buf(noalloc, alloc, buf, 4, obj);
    }
    if (tag == 0xDC) {
        // array 16
        return decode_len_array(noalloc, alloc, buf, 2, obj);
    }
    if (tag == 0xDD) {
        // array 32
        return decode_len_array(noalloc, alloc, buf, 4, obj);
    }
    if (tag == 0xDE) {
        // map 16
        return decode_len_map(noalloc, alloc, buf, 2, obj);
    }
    if (tag == 0xDF) {
        // map 32
        return decode_len_map(noalloc, alloc, buf, 4, obj);
    }
    // negative fixint
    int8_t val;
    memcpy(&val, &tag, 1);
    *obj = GRAVEL_OBJ_I64(val);
    return 0;
}

int gravel_msgpack_decode(
    GravelAlloc *alloc, GravelBuffer buf, GravelObject *obj
) {
    assert((alloc != NULL) && (obj != NULL));

    GravelBuffer msg = buf;
    int ret = decode_obj(false, alloc, &msg, obj);
    if (ret != 0) {
        return ret;
    }

    // Ensure no trailing data
    if (msg.len != 0) {
        GRAVEL_LOGE("msgpack", "Payload has %zu trailing bytes.", msg.len);
        return EBADMSG;
    }

    return 0;
}

int gravel_msgpack_decode_lazy_noalloc(GravelBuffer *buf, GravelObject *obj) {
    assert((buf != NULL) && (obj != NULL));

    GravelAlloc alloc = { 0 }; // never used when noalloc is true

    return decode_obj(true, &alloc, buf, obj);
}
