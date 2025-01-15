#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rbcc.h"
str file_name_with_suffix(str file_name, str suffix) {
    size_t pos_found = SIZE_MAX;
    bool   found     = false;
    for (size_t i = file_name.len - 1; i >= 0; i--) {
        if (file_name.data[i] == '.') {
            // test.rbc
            //     ^
            //     4
            pos_found = i;
            found     = true;
            break;
        } else if (file_name.data[i] == '/' || file_name.data[i] == '\\') {
            // Found a path limiter, there is no suffix to find
            break;
        }
    }

    if (!found) {
        if (str_eq(suffix, S(""))) {
            return alloc_print_str("%s", file_name.data);
        }
        return alloc_print_str("%s.%s", file_name.data, suffix.data);
    }

    if (str_eq(suffix, S(""))) {
        u8 *buffer = xmalloc(pos_found + 1);
        memcpy(buffer, file_name.data, pos_found);
        buffer[pos_found] = 0;
        return (str){.data = buffer, .len = pos_found};
    }

    size_t len_existing_suffix = file_name.len - pos_found - 1;
    size_t to_allocate =
        (file_name.len - len_existing_suffix) + suffix.len + 1 /* \0 */;
    u8 *buffer = xmalloc(to_allocate);
    memcpy(buffer, file_name.data, pos_found + 1);
    memcpy(buffer + pos_found + 1, suffix.data, suffix.len);
    buffer[to_allocate - 1] = 0;
    return (str){.data = buffer, .len = to_allocate - 1};
}
