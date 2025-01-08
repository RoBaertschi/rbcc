#pragma once

#include <stddef.h>
#include "rbcc.h"

// Typedefs
typedef struct ir_program             ir_program;
typedef struct ir_function            ir_function;
typedef struct ir_value               ir_value;
typedef struct ir_instruction         ir_instruction;
typedef struct ir_instructions        ir_instructions;
typedef struct ir_instructions_buffer ir_instructions_buffer;

struct ir_instructions {
    ir_instruction *NULLABLE data;
    size_t                   len;
};

// Frees the buffer
ir_instructions ir_instructions_new(ir_instructions_buffer buffer);
void            ir_instructions_free(ir_instructions vec);
void            ir_instructions_free_all(ir_instructions vec);

struct ir_instructions_buffer {
    ir_instruction *NULLABLE data;
    size_t                   len;
    size_t                   cap;
};

void ir_instructions_buffer_push(ir_instructions_buffer *NONNULL buffer,
                                 ir_instruction                  inst);

// Appends and frees a ir_instructions list
void ir_instructions_buffer_append(ir_instructions_buffer *NONNULL buffer,
                                   ir_instructions                 list);
ir_instructions_buffer ir_instructions_buffer_new(size_t initial_cap);
void ir_instructions_buffer_free(ir_instructions_buffer buffer);
void ir_instructions_buffer_free_all(ir_instructions_buffer buffer);

struct ir_program {
    ir_function *NONNULL main_function;
};

void ir_program_print(ir_program *NONNULL program);
void ir_program_free(ir_program *NONNULL program);

struct ir_function {
    str             name;
    ir_instructions instructions;
};

void ir_function_print(ir_function *NONNULL function);
void ir_function_free(ir_function *NONNULL function);

struct ir_value {
    enum ir_value_kind { value_constant } tag;
    union {
        struct value_constant {
            i64 value;
        } value_constant;
    } data;
};

#define IR_VALUE_NEW(kind, ...) \
    ir_value_new((ir_value){kind, {(struct kind){__VA_ARGS__}}})

void              ir_value_print(ir_value *NONNULL value);
ir_value *NONNULL ir_value_new(ir_value value);
void              ir_value_free(ir_value *NONNULL value);

struct ir_instruction {
    enum ir_instruction_kind {
        INST_RET, // uses lhs for value, ignores rhs
    } kind;
    ir_value *NULLABLE lhs, *NULLABLE rhs;
};

void           ir_instruction_print(ir_instruction *NONNULL inst);
ir_instruction ir_instruction_new(enum ir_instruction_kind kind,
                                  ir_value *NULLABLE       lhs,
                                  ir_value *NULLABLE       rhs);

void           ir_instruction_free(ir_instruction inst);
