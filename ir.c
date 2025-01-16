#include "ir.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbcc.h"

ir_instructions ir_instructions_new(ir_instructions_buffer buffer) {
    ir_instructions vec = {
        .len  = buffer.len,
        .data = buffer.len > 0 ? xmalloc(sizeof(ir_instruction) * buffer.len)
                               : NULL,
    };

    memcpy(vec.data, buffer.data, sizeof(ir_instruction) * buffer.len);
    ir_instructions_buffer_free(buffer);

    return vec;
}
void ir_instructions_free(ir_instructions vec) { free(vec.data); }
void ir_instructions_free_all(ir_instructions vec) {
    for (size_t i = 0; i < vec.len; i++) {
        ir_instruction_free(vec.data[i]);
    }
    ir_instructions_free(vec);
}

static void
ir_instructions_buffer_ensure_size(ir_instructions_buffer *NONNULL buffer,
                                   size_t                          len) {
    while (buffer->cap < len) {
        buffer->data =
            realloc(buffer->data, buffer->cap * sizeof(ir_instruction) * 2);
        CHECK_ALLOC(buffer->data);
        buffer->cap = buffer->cap * 2;
    }
}

void ir_instructions_buffer_push(ir_instructions_buffer *NONNULL buffer,
                                 ir_instruction                  inst) {
    ir_instructions_buffer_ensure_size(buffer, buffer->len + 1);
    buffer->data[buffer->len] = inst;
    buffer->len += 1;
}

void ir_instructions_buffer_append(ir_instructions_buffer *NONNULL buffer,
                                   ir_instructions                 list) {
    ir_instructions_buffer_ensure_size(buffer, buffer->len + list.len);
    memcpy(buffer->data + buffer->len, list.data, list.len * sizeof(ir_instruction));
    buffer->len += list.len;
    ir_instructions_free(
        list); // we take ownership of all the values in the list,
               // because of that, we don't have to free the data
}

ir_instructions_buffer ir_instructions_buffer_new(size_t initial_cap) {
    ir_instructions_buffer buffer = {
        .len  = 0,
        .cap  = initial_cap,
        .data = initial_cap > 0 ? xmalloc(sizeof(ir_instruction) * initial_cap)
                                : NULL,
    };

    return buffer;
}
void ir_instructions_buffer_free(ir_instructions_buffer buffer) {
    free(buffer.data);
}

void ir_instructions_buffer_free_all(ir_instructions_buffer buffer) {
    for (size_t i = 0; i < buffer.len; i++) {
        ir_instruction_free(buffer.data[i]);
    }
    ir_instructions_buffer_free(buffer);
}

void ir_program_print(ir_program *NONNULL program) {
    ir_function_print(program->main_function);
}

void ir_program_free(ir_program *NONNULL program) {
    ir_function_free(program->main_function);
}

void ir_function_print(ir_function *NONNULL function) {
    ir_function func = *function;
    printf("function %s:\n", func.name.data);
    if (!func.instructions.data) {
        return;
    }
    for (size_t i = 0; i < func.instructions.len; i++) {
        ir_instruction_print(&func.instructions.data[i]);
    }
}

void ir_function_free(ir_function *NONNULL function) {
    ir_instructions_free_all(function->instructions);
    str_free(function->name);
    free(function);
}

void ir_value_print(ir_value *NONNULL value) {
    ir_value v = *value;
    switch (v.tag) {
        case value_constant: {
            struct value_constant data = v.data.value_constant;
            printf("%ld", data.value);
            break;
        }
        case value_temp: {
            struct value_temp data = v.data.value_temp;
            printf("%%%s", data.value.data);
            break;
        }
    }
}

ir_value *NONNULL ir_value_new(ir_value value) {
    ir_value *ptr = xmalloc(sizeof(ir_value));
    *ptr          = value;
    return ptr;
}

void ir_value_free(ir_value *NONNULL value) { free(value); }

void ir_value_print_invalid(ir_value *NULLABLE value) {
    if (value) {
        ir_value_print(value);
    } else {
        printf("(invalid operand)");
    }
}

void ir_instruction_print(ir_instruction *NONNULL inst) {
    ir_instruction i    = *inst;

    char const    *temp = "(unset temp)";

    switch (i.kind) {
        case INST_RET:
            printf("  RET ");
            if (i.lhs) {
                ir_value_print(i.lhs);
            } else {
                printf("(invalid operand)");
            }

            printf("\n");
            break;

        // Binary Instructions
        case INST_ADD:
            temp = "ADD";
            goto print_binary;
        case INST_SUB:
            temp = "SUB";
            goto print_binary;
        case INST_MUL:
            temp = "MUL";
            goto print_binary;
        case INST_DIV:
            temp = "DIV";
            goto print_binary;

        print_binary:
            printf("  ");
            ir_value_print_invalid(i.dst);
            printf(" = %s ", temp);
            ir_value_print_invalid(i.lhs);
            printf(", ");
            ir_value_print_invalid(i.rhs);
            printf("\n");
            break;
    }
}

ir_instruction ir_instruction_new(enum ir_instruction_kind kind,
                                  ir_value *NULLABLE       lhs,
                                  ir_value *NULLABLE       rhs,
                                  ir_value *NULLABLE       dst) {
    return (ir_instruction){.kind = kind, .lhs = lhs, .rhs = rhs, .dst = dst};
}

void ir_instruction_free(ir_instruction inst) {
    if (inst.lhs != NULL) {
        ir_value_free(inst.lhs);
    }
    if (inst.rhs != NULL) {
        ir_value_free(inst.rhs);
    }
    if (inst.dst != NULL) {
        ir_value_free(inst.dst);
    }
}
