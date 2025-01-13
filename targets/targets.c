#include "targets.h"
#include "targets/x86_64-linux.h"

target get_default_target(void) { return TARGET_X86_64_LINUX; }

void   code_gen(char const *NONNULL file_name, target target,
                ir_program program) {
    switch (target) {
        case TARGET_X86_64_LINUX:
            x86_64_linux_emit_code(program, file_name);
            break;
    }
}
