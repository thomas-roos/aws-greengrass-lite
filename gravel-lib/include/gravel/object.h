/* gravel - Utilities for AWS IoT Core clients
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates
 */

#ifndef GRAVEL_OBJECT_H
#define GRAVEL_OBJECT_H

/*! Generic dynamic object representation. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Union tag for `Gravel_Object`. */
enum GravelObjectType {
    GRAVEL_TYPE_NULL = 0,
    GRAVEL_TYPE_BOOLEAN,
    GRAVEL_TYPE_U64,
    GRAVEL_TYPE_I64,
    GRAVEL_TYPE_F64,
    GRAVEL_TYPE_BUF,
    GRAVEL_TYPE_LIST,
    GRAVEL_TYPE_MAP,
};

/** A fixed buffer of bytes. Possibly a string. */
typedef struct {
    uint8_t *data;
    size_t len;
} GravelBuffer;

/** An array of `Gravel_Object`. */
typedef struct {
    struct GravelObject *items;
    size_t len;
} GravelList;

/** A map of UTF-8 strings to `Gravel_Object`s. */
typedef struct {
    struct GravelKV *pairs;
    size_t len;
} GravelMap;

/** A generic object. */
typedef struct GravelObject {
    enum GravelObjectType type;

    union {
        bool boolean;
        uint64_t u64;
        int64_t i64;
        double f64;
        GravelBuffer buf;
        GravelList list;
        GravelMap map;
    };
} GravelObject;

/** A key-value pair used for `Gravel_Map`.
 * `key` must be an UTF-8 encoded string. */
typedef struct GravelKV {
    GravelBuffer key;
    GravelObject val;
} GravelKV;

// statement expression needed as only way to tell if an expression is a string
// literal is to assign it to a char array declaration. Note that a initializer
// expression for array of char would also pass this, which is unfortunate.

/** Create buffer literal from a string literal. */
#define GRAVEL_STR(strlit) \
    __extension__({ \
        char temp[] __attribute((unused)) = strlit; \
        (GravelBuffer) { \
            .data = (uint8_t *) strlit, \
            .len = sizeof(strlit) - 1U, \
        }; \
    })

// generic function on pointer is to validate parameter is array and not ptr.
// On systems where char == uint8_t, this won't warn on string literal.

/** Create buffer literal from a byte array. */
#define GRAVEL_BUF(...) \
    _Generic( \
        (&(__VA_ARGS__)), \
        uint8_t(*)[]: ((GravelBuffer) { .data = (__VA_ARGS__), \
                                        .len = sizeof(__VA_ARGS__) }) \
    )

/** Create list literal from object literals. */
#define GRAVEL_LIST(...) \
    (GravelList) { \
        .items = (GravelObject[]) { __VA_ARGS__ }, \
        .len = (sizeof((GravelObject[]) { __VA_ARGS__ })) \
            / (sizeof(GravelObject)) \
    }

/** Create map literal from key-value literals. */
#define GRAVEL_MAP(...) \
    (GravelMap) { \
        .pairs = (GravelKV[]) { __VA_ARGS__ }, \
        .len = (sizeof((GravelKV[]) { __VA_ARGS__ })) / (sizeof(GravelKV)) \
    }

/** Create null object literal. */
#define GRAVEL_OBJ_NULL() \
    (GravelObject) { \
        .type = GRAVEL_TYPE_NULL \
    }

/** Create bool object literal. */
#define GRAVEL_OBJ_BOOL(value) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_BOOLEAN, .boolean = value \
    }

/** Create unsigned integer object literal. */
#define GRAVEL_OBJ_U64(value) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_U64, .u64 = value \
    }

/** Create signed integer object literal. */
#define GRAVEL_OBJ_I64(value) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_I64, .i64 = value \
    }

/** Create floating point object literal. */
#define GRAVEL_OBJ_F64(value) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_F64, .f64 = value \
    }

/** Create buffer object literal from a string literal. */
#define GRAVEL_OBJ_STR(strlit) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_BUF, .buf = GRAVEL_STR(strlit), \
    }

/** Create buffer object literal from a byte array. */
#define GRAVEL_OBJ_BUF(...) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_BUF, .buf = GRAVEL_BUF(__VA_ARGS__), \
    }

/** Create map object literal from `Gravel_KV` literals. */
#define GRAVEL_OBJ_MAP(...) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_MAP, .map = GRAVEL_MAP(__VA_ARGS__), \
    }

/** Create list object literal from object literals. */
#define GRAVEL_OBJ_LIST(...) \
    (GravelObject) { \
        .type = GRAVEL_TYPE_LIST, .list = GRAVEL_LIST(__VA_ARGS__), \
    }

#define GRAVEL_FORCE(type, value) \
    _Generic((value), type: (value), default: (type) { 0 })

/** Create object literal from buffer, list, or map. */
#define GRAVEL_OBJ(...) \
    _Generic( \
        (__VA_ARGS__), \
        GravelBuffer: (GravelObject \
        ) { .type = GRAVEL_TYPE_BUF, \
            .buf = GRAVEL_FORCE(GravelBuffer, (__VA_ARGS__)) }, \
        GravelList: (GravelObject \
        ) { .type = GRAVEL_TYPE_LIST, \
            .list = GRAVEL_FORCE(GravelList, (__VA_ARGS__)) }, \
        GravelMap: (GravelObject) { .type = GRAVEL_TYPE_MAP, \
                                    .map \
                                    = GRAVEL_FORCE(GravelMap, (__VA_ARGS__)) } \
    )

#endif
