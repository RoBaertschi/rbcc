#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbcc.h"

char *NONNULL alloc_print(char const *fmt, ...) {
    va_list arg1, arg2;
    va_start(arg1, fmt);
    va_copy(arg2, arg1);
    int buffer_size = vsnprintf(NULL, 0, fmt, arg1) + 1;
    va_end(arg1);
    char *buffer = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    va_end(arg2);
    return buffer;
}

struct str PRINTF_FORMAT(1, 2) alloc_print_str(char const *NONNULL fmt, ...) {
    va_list arg1, arg2;
    va_start(arg1, fmt);
    va_copy(arg2, arg1);
    int buffer_size = vsnprintf(NULL, 0, fmt, arg1) + 1;
    va_end(arg1);
    char *buffer = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    va_end(arg2);
    return (str){.data = (u8 *)buffer, .len = buffer_size - 1};
}

bool str_eq(str str1, str str2) {
    if (str1.len != str2.len) {
        return false;
    }

    for (size_t i = 0; i < str1.len; i++) {
        if (str1.data[i] != str2.data[i]) {
            return false;
        }
    }

    return true;
}

str str_slice_clone(str_slice s) {
    u8 *buffer = xmalloc(s.len + 1);
    memcpy(buffer, s.data, s.len);
    buffer[s.len] = 0; // '\0' byte
    return (str){.data = buffer, .len = s.len};
}

str str_clone(str s) {
    u8 *buffer = xmalloc(s.len + 1);
    memcpy(buffer, s.data, s.len);
    buffer[s.len] = 0; // '\0' byte
    return (str){.data = buffer, .len = s.len};
}

void str_free(str s) { free(s.data); }
