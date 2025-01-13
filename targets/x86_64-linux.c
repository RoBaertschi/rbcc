#include "x86_64-linux.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "rbcc.h"

typedef struct state {
    FILE *NONNULL file;
} state;

void PRINTF_FORMAT(1, 2) fail(char const *NONNULL msg, ...) {
    va_list arg;
    va_start(arg, msg);

    fprintf(stderr, "(x86_64-linux) asm generation failed: ");
    vfprintf(stderr, msg, arg);
    fprintf(stderr, "\n");
    exit(1);

    va_end(arg);
}

void emitf(state *NONNULL s, char const *NONNULL msg, ...) {
    va_list arg;
    va_start(arg, msg);
    vfprintf(s->file, msg, arg);
    va_end(arg);
}

static void emit_function(state *s, ir_function *func);
static void emit_program(state *s, ir_program prog);
static void emit_instruction(state *s, ir_instruction inst);
static char *get_value(ir_value* value);

void        x86_64_linux_emit_code(ir_program program, char const *file_name) {
    errno   = 0;
    state s = {
               .file = fopen(file_name, "w+"),
    };
    if (s.file == NULL) {
        fail("Could not open file %s because: %s", file_name, strerror(errno));
    }
    emitf(&s, "format ELF64\nsection '.text' executable\n");
    emit_program(&s, program);
    fclose(s.file);
}

static void emit_program(state *NONNULL s, ir_program prog) {
    emit_function(s, prog.main_function);

    // TODO: Clear up what we need to setup for libc and use our own main
    // Read this for argv and argc: http://dbp-consulting.com/tutorials/debugging/linuxProgramStartup.html
    /*emitf(s, */
    /*      "public _start\n"*/
    /*      "_start:\n"*/
    /*      "  xor rbp,rbp\n"*/
    /*      "  call %s\n"*/
    /*      "  mov rdi, rax\n"*/
    /*      "  mov rax, 60\n"*/
    /*      "  syscall\n",*/
    /*      prog.main_function->name.data);*/
}

static void emit_function(state *NONNULL s, ir_function *NONNULL func) {
    emitf(s, "public %s\n", func->name.data);
    emitf(s, "%s:\n", func->name.data);

    for (size_t i = 0; i < func->instructions.len; i++) {
        emit_instruction(s, func->instructions.data[i]);
    }
}

static void emit_instruction(state *NONNULL s, ir_instruction inst) {
    switch (inst.kind) {
        case INST_RET: {
            if (!inst.lhs) {
                fail("invalid ir ret instruction, lhs is null");
            }
            char *lhs = get_value(inst.lhs);
            emitf(s, "  mov rax,%s\n", lhs);
            free(lhs);
            emitf(s, "  ret\n");
            break;
        }
    }
}

static char *get_value(ir_value *NONNULL ptr) {
    ir_value value = *ptr;
    switch (value.tag) {
        case value_constant: {
            struct value_constant data = value.data.value_constant;
            return alloc_print("%ld", data.value);
        }
    }
}
