#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbcc.h"

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
