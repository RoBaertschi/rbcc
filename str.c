#include <stdlib.h>
#include <string.h>
#include "rbcc.h"

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
