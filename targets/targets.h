#pragma once

typedef enum target {
    TARGET_X86_64_LINUX,
} target;

target get_default_target(void);
