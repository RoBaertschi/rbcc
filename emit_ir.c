#include "emit_ir.h"
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "ir.h"
#include "rbcc.h"

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
            ir_instructions_buffer_push(&b, ir_instruction_new(INST_RET, expr.result, NULL));

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
