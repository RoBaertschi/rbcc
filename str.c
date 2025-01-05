#include <stdlib.h>
#include <string.h>
#include "rbcc.h"

str str_clone(str s) {
    u8 *buffer = xmalloc(s.len);
    memcpy(buffer, s.data, s.len);
    return (str){.data = buffer, .len = s.len};
}

void str_free(str s) { free(s.data); }
