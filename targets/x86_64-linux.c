#include "x86_64-linux.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "da.h"
#include "ir.h"
#include "rbcc.h"

typedef struct asm_operand {
    enum asm_operand_tag {
        asm_op_pseudo,
        asm_op_imm,
        asm_op_stack,
        asm_op_register,
    } tag;

    union {
        struct asm_op_pseudo {
            str value;
        } asm_op_pseudo;
        struct asm_op_imm {
            i64 value;
        } asm_op_imm;
        struct asm_op_stack {
            i64 value;
        } asm_op_stack;
        struct asm_op_register {
            enum asm_register {
                REG_AX,
                REG_R9,
                REG_R10,
            } value;
        } asm_op_register;
    } data;
} asm_operand;

asm_operand *asm_operand_new(asm_operand operand) {
    asm_operand *ptr = malloc(sizeof(asm_operand));
    *ptr             = operand;
    return ptr;
}

void asm_operand_free(asm_operand *ptr) { free(ptr); }

#define OP_NEW(tag, ...) \
    asm_operand_new((asm_operand){tag, {.tag = (struct tag){__VA_ARGS__}}})

typedef struct asm_instruction {
    enum asm_instruction_tag {
        ASM_INST_MOV,
        ASM_INST_ADD,
        ASM_INST_SUB,
        ASM_INST_IDIV,
        ASM_INST_MUL,
        ASM_INST_RET,
    } tag;
} asm_instruction;

typedef struct asm_instructions {
    asm_instruction *items;
    size_t           count;
    size_t           capacity;
} asm_instructions;

typedef struct asm_function {
    str              name;
    asm_instructions insts;
} asm_function;

typedef struct asm_program {
    asm_function function;
} asm_program;

static asm_instructions cg_instruction(ir_instruction inst) {
    asm_instructions insts = {0};
    switch (inst.kind) {
        case INST_RET: {
            asm_instruction ret = {
                .tag = ASM_INST_RET,
            };
            da_append(&insts, ret);
            break;
        }
        enum asm_instruction_tag tag;
        case INST_ADD:
            tag = ASM_INST_ADD;
            goto binary;
        case INST_SUB:
            tag = ASM_INST_SUB;
            goto binary;
        case INST_MUL:
            tag = ASM_INST_MUL;
            goto binary;
        case INST_DIV:
            tag = ASM_INST_IDIV;
            goto binary;
        binary:
    }

    return insts;
}

static asm_function cg_function(ir_function* func) {
    asm_instructions insts = {0};
    for (size_t i = 0; i < func->instructions.len; i++) {
        asm_instructions insts = cg_instruction(func->instructions.data[i]);
        da_append_list(&insts, &insts);
    }

    return (asm_function){.insts = insts, .name = str_clone(func->name)};
}

static asm_program cg_program(ir_program prog) {
    return (asm_program){.function = cg_function(prog.main_function)};
}


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

static void  emit_function(state *s, ir_function *func);
static void  emit_program(state *s, ir_program prog);
static void  emit_instruction(state *s, ir_instruction inst);
static char *get_value(ir_value *value);

void         x86_64_linux_emit_code(ir_program program, char const *file_name) {
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
    // Read this for argv and argc:
    // http://dbp-consulting.com/tutorials/debugging/linuxProgramStartup.html
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
        case INST_ADD:
        case INST_SUB:
        case INST_MUL:
        case INST_DIV:
            break;
    }
}

static char *get_value(ir_value *NONNULL ptr) {
    ir_value value = *ptr;
    switch (value.tag) {
        case value_constant: {
            struct value_constant data = value.data.value_constant;
            return alloc_print("%ld", data.value);
        }
        case value_temp:
            break;
    }
}
