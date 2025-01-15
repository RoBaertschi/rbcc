#include "ast.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "rbcc.h"

void program_print(program *NONNULL prog) {
    printf("program(");
    if (prog->main_function) {
        stmt_print(prog->main_function);
    } else {
        printf("null");
    }
    printf(")");
}

program *program_new(program prog) {
    program *ptr = xmalloc(sizeof(program));
    *ptr         = prog;
    return ptr;
}

void program_free(program *NULLABLE prog) {
    if (!prog) {
        return;
    }
    stmt_free(prog->main_function);
    free(prog);
}

void stmt_print(stmt *NONNULL ptr) {
    stmt s = *ptr;
    switch (s.tag) {
        case stmt_function: {
            struct stmt_function data = s.data.stmt_function;
            printf("stmt_function(name = %s, body = ", data.name.data);
            expr_print(data.body);
            printf(")");
            return;
        }
    }
}

stmt *NONNULL stmt_new(stmt s) {
    stmt *ptr = xmalloc(sizeof(stmt));
    *ptr      = s;
    return ptr;
}

void stmt_free(stmt *ptr) {
    if (!ptr) {
        return;
    }

    stmt s = *ptr;
    switch (s.tag) {
        case stmt_function: {
            struct stmt_function data = s.data.stmt_function;
            expr_free(data.body);
            str_free(data.name);
            free(ptr);
            return;
        }
    }
}

expr_list_buffer expr_list_buffer_new(size_t initial_cap) {
    if (initial_cap <= 0) {
        initial_cap = 2;
    }
    return (expr_list_buffer){
        .len  = 0,
        .cap  = initial_cap,
        .data = xmalloc(sizeof(expr *) * initial_cap),
    };
}

void expr_list_buffer_push(expr_list_buffer *NONNULL buffer, expr *NULLABLE e) {
    if (buffer->cap <= buffer->len) {
        buffer->cap *= 2;
        buffer->data = realloc(buffer->data, sizeof(expr *) * buffer->cap);
    }
    buffer->data[buffer->len] = e;
    buffer->len += 1;
}

void expr_list_buffer_free(expr_list_buffer buffer) { free(buffer.data); }

void expr_list_buffer_free_all(expr_list_buffer buffer) {
    for (size_t i = 0; i < buffer.len; i++) {
        expr_free(buffer.data[i]);
    }
    expr_list_buffer_free(buffer);
}

void expr_list_print(expr_list *NONNULL list) {
    for (size_t i = 0; i < list->len; i++) {
        expr_print(list->data[i]);
        if (i + 1 < list->len) {
            printf(", ");
        }
    }
}

expr_list expr_list_new(expr_list_buffer const buffer) {
    assert(0 < buffer.len);
    expr_list list = {0};
    list.data      = xmalloc(sizeof(expr) * buffer.len);
    memcpy(list.data, buffer.data, sizeof(expr) * buffer.len);
    list.len = buffer.len;
    return list;
}

void expr_list_free(expr_list list) {
    for (size_t i = 0; i < list.len; i++) {
        expr_free(list.data[i]);
    }
    free(list.data);
}

char const *const binary_operator_strs[] = {
#define _X(op, symbol) [op] = #symbol,
    BINARY_OPERATORS
#undef _X
};

char const *binary_operator_str(binary_operator op) {
    if (op >= BOP_MAX || op < 0) {
        return "UNKNOWN BINARY OPERATOR";
    }
    return binary_operator_strs[op];
}

void expr_print(expr *NONNULL ptr) {
    expr e = *ptr;
    switch (e.tag) {
        case expr_string: {
            struct expr_string data = e.data.expr_string;
            printf("expr_string(\"%s\")", data.content.data);
            return;
        }
        case expr_function_call: {
            struct expr_function_call data = e.data.expr_function_call;
            printf("expr_function_call(");
            expr_list_print(&data.params);
            printf(")");
            return;
        }
        case expr_constant: {
            struct expr_constant data = e.data.expr_constant;
            printf("expr_constant(%ld)", data.value);
            return;
        }
        case expr_binary: {
            struct expr_binary data = e.data.expr_binary;
            printf("expr_binary(");
            expr_print(data.lhs);
            printf(" %s ", binary_operator_str(data.op));
            expr_print(data.rhs);
            printf(")");
            return;
        }
    }
}

expr *NONNULL expr_new(expr e) {
    expr *ptr = xmalloc(sizeof(expr));
    *ptr      = e;
    return ptr;
}

void expr_free(expr *NULLABLE ptr) {
    if (!ptr) {
        return;
    }

    expr e = *ptr;

    lexer_token_free(e.root_token);
    switch (e.tag) {
        case expr_string: {
            struct expr_string data = e.data.expr_string;
            str_free(data.content);
            free(ptr);
            return;
        }
        case expr_function_call: {
            struct expr_function_call data = e.data.expr_function_call;
            expr_list_free(data.params);
            free(ptr);
            return;
        }
        case expr_constant: {
            /*struct expr_constant data = e.data.expr_constant;*/
            free(ptr);
            return;
        }
        case expr_binary: {
            struct expr_binary data = e.data.expr_binary;
            expr_free(data.lhs);
            expr_free(data.rhs);
            free(ptr);
            return;
        }
    }
}
