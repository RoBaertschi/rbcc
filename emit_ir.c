#include "emit_ir.h"
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "ir.h"
#include "rbcc.h"

ir_value *make_temp(void) {
    str temp = str_unique();
    return IR_VALUE_NEW(value_temp, temp);
}

ir_value *make_copy(ir_value *ptr) {
    ir_value value = *ptr;
    switch (value.tag) {
        case value_constant: {
            /*struct value_constant data = value.data.value_constant;*/
            return ptr;
        }
        case value_temp: {
            struct value_temp data = value.data.value_temp;
            return IR_VALUE_NEW(value_temp, str_clone(data.value));
        }
    }
}

typedef struct ir_expr {
    ir_instructions   insts;
    ir_value *NONNULL result;
} ir_expr;

ir_expr ir_emit_expr(expr *ptr) {
    expr e = *ptr;
    switch (e.tag) {
        case expr_constant: {
            struct expr_constant data = e.data.expr_constant;
            return (ir_expr){
                .insts  = {0},
                .result = IR_VALUE_NEW(value_constant, data.value),
            };
        }
        case expr_binary: {
            struct expr_binary     data     = e.data.expr_binary;

            ir_instructions_buffer buffer   = ir_instructions_buffer_new(1);

            ir_expr                lhs_expr = ir_emit_expr(data.lhs);
            ir_instructions_buffer_append(&buffer, lhs_expr.insts);
            ir_expr                rhs_expr = ir_emit_expr(data.rhs);
            ir_instructions_buffer_append(&buffer, rhs_expr.insts);

            ir_value              *lhs      = make_copy(lhs_expr.result);
            ir_value              *rhs      = make_copy(rhs_expr.result);
            ir_value              *dst      = make_temp();

            ir_instructions_buffer_push(
                &buffer,
                ir_instruction_new(data.op + INST_ADD, lhs, rhs, dst));

            return (ir_expr){.insts  = ir_instructions_new(buffer),
                             .result = make_copy(dst)};
        }
        case expr_string:
        case expr_function_call:
            printf("Reached unimplemented expr ir emitission");
            return (ir_expr){
                .insts =
                    {
                            .data = NULL,
                            .len  = 0,
                            },
                .result = IR_VALUE_NEW(value_constant, 0),
            };
    }
}

ir_function *NONNULL ir_emit_function(stmt *ptr) {
    stmt s = *ptr;
    switch (s.tag) {
        case stmt_function: {
            struct stmt_function   data = s.data.stmt_function;
            ir_expr                expr = ir_emit_expr(data.body);
            ir_instructions_buffer b    = ir_instructions_buffer_new(1);
            ir_instructions_buffer_append(&b, expr.insts);
            ir_instructions_buffer_push(
                &b, ir_instruction_new(INST_RET, expr.result, NULL, NULL));

            ir_instructions insts = ir_instructions_new(b);

            ir_function    *ptr   = xmalloc(sizeof(ir_function));
            *ptr                  = (ir_function){.name         = str_clone(data.name),
                                                  .instructions = insts};
            return ptr;
        }
    }
}

ir_program ir_emit_program(program *NONNULL prog) {
    return (ir_program){
        .main_function = ir_emit_function(prog->main_function),
    };
}
