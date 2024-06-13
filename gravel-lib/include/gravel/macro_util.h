/* gravel - Utilities for AWS IoT Core clients
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GRAVEL_MACRO_UTIL_H
#define GRAVEL_MACRO_UTIL_H

/** Expands to first argument. */
#define GRAVEL_MACRO_FIRST(first, ...) first

/** Expands to args, skipping first. */
#define GRAVEL_MACRO_REST(first, ...) __VA_ARGS__

/** Combine two args, expanding them. */
#define GRAVEL_MACRO_PASTE(left, right) GRAVEL_MACRO_PASTE2(left, right)
#define GRAVEL_MACRO_PASTE2(left, right) left##right

#endif
