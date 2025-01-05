#include "ast.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "rbcc.h"

stmt *stmt_new(stmt s) {
    stmt *ptr = xmalloc(sizeof(stmt));
    *ptr      = s;
    return ptr;
}

void stmt_free(stmt *ptr) {
    stmt s = *ptr;
    switch (s.tag) {
        case stmt_function: {
            struct stmt_function data = s.data.stmt_function;
            expr_free(data.body);
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

void expr_list_buffer_push(expr_list_buffer *buffer, expr *e) {
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

expr_list expr_list_new(expr_list_buffer const buffer) {
    expr_list list = {0};
    list.data      = xmalloc(sizeof(expr) * buffer.len);
    memcpy(list.data, buffer.data, buffer.len);
    list.len = buffer.len;
    return list;
}

void expr_list_free(expr_list list) {
    for (size_t i = 0; i < list.len; i++) {
        expr_free(list.data[i]);
    }
    free(list.data);
}

expr *expr_new(expr e) {
    expr *ptr = xmalloc(sizeof(expr));
    *ptr      = e;
    return ptr;
}

void expr_free(expr *ptr) {
    expr e = *ptr;
    switch (e.tag) {
        case expr_string: {
            struct expr_string data = e.data.expr_string;
            str_free(data.content);
            return;
        }
        case expr_function_call: {
            struct expr_function_call data = e.data.expr_function_call;
            expr_list_free(data.params);
            return;
        }
    }
}
