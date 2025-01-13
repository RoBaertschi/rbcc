#pragma once

#include "ir.h"
#include "rbcc.h"

typedef enum target {
    TARGET_X86_64_LINUX,
} target;

target get_default_target(void);

void code_gen(char const *NONNULL file_name, target target, ir_program program);
