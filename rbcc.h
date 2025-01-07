#pragma once

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_FORMAT(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define PRINTF_FORMAT(x, y)
#endif

#ifdef __clang__
#define NULLABLE _Nullable
#define NONNULL _Nonnull
#else
#define NULLABLE
#define NONNULL
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef float    f32;
typedef double   f64;

static_assert(sizeof(u8) == sizeof(char),
              "Expected the size of char to be 1 byte.");

#define CHECK_ALLOC(value)                                                \
    if (value == NULL) {                                                  \
        fprintf(stderr,                                                   \
                "Memory allocation failed, probably out of memory, last " \
                "error: %s\n",                                            \
                strerror(errno));                                         \
        abort();                                                          \
    }

inline void *NONNULL xmalloc(size_t size) {
    void *result = malloc(size);
    CHECK_ALLOC(result);
    return result;
}

// Strings
// Strings are assumed to be utf8, but are not enforced.
// For compatabilty with c, a string still contains a '\0'
// Also, strings can or cannot own memory, that depends on how they are created,
// to be sure, clone the string. len excludes the '\0'
typedef struct str {
    u8 *NULLABLE data;
    size_t       len;
} str;

// String slice
// Does _NOT_ end with a '\0'.
// It references to a other string. Be carefull to not use a slice while
// the string is already freed.
typedef struct str_slice {
    u8 *NULLABLE data;
    size_t       len;
} str_slice;

#define S(s) \
    (str) { .data = s, .len = sizeof(s) - 1 }

str  str_slice_clone(str_slice slice);
str  str_clone(str s);
void str_free(str s);
