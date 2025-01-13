#pragma once

#include <stddef.h>
#include "lexer.h"
#include "rbcc.h"
// AST
typedef struct program program;

// Statements
typedef struct stmt stmt;

// Expressions
typedef struct expr expr;

typedef struct expr_list_buffer {
    expr *NONNULL *NULLABLE data;
    size_t                  len;
    size_t                  cap;
} expr_list_buffer;

expr_list_buffer expr_list_buffer_new(size_t initial_cap);
void             expr_list_buffer_push(expr_list_buffer *NONNULL buffer,
                                       expr *NULLABLE            expr);
// Frees the buffer itself, not the expr it contains
void expr_list_buffer_free(expr_list_buffer buffer);
// Frees the buffer and all the expr it contains
void expr_list_buffer_free_all(expr_list_buffer buffer);

typedef struct expr_list {
    expr *NONNULL *NULLABLE data;
    size_t                  len;
} expr_list;

void expr_list_print(expr_list *NONNULL list);
// Makes a copy of the buffer, does _NOT_ free the buffer.
expr_list expr_list_new(expr_list_buffer const buffer);
void      expr_list_free(expr_list list);

struct program {
    struct stmt *NULLABLE main_function;
};

void             program_print(program *NONNULL prog);
program *NONNULL program_new(program prog);
void             program_free(program *NULLABLE prog);

struct stmt {
    enum {
        stmt_function, // Function definition
    } tag;
    union {
        struct stmt_function {
            str            name;
            expr *NULLABLE body;
        } stmt_function;
    } data;
};

void          stmt_print(stmt *NONNULL stmt);
stmt *NONNULL stmt_new(stmt stmt);
void          stmt_free(stmt *NULLABLE stmt);

#define STMT_NEW(tag, ...) \
    stmt_new((stmt){tag, {.tag = (struct tag){__VA_ARGS__}}})

struct expr {
    enum {
        expr_string,
        expr_constant,
        expr_function_call,
    } tag;
    token root_token;
    union {
        struct expr_string {
            str content; // this is owned
        } expr_string;
        struct expr_constant {
            i64 value;
        } expr_constant;
        struct expr_function_call {
            expr_list params;
        } expr_function_call;
    } data;
};

void          expr_print(expr *NONNULL expr);
expr *NONNULL expr_new(expr expr);
void          expr_free(expr *NULLABLE expr);

#define EXPR_NEW(tag, tok, ...) \
    expr_new((expr){            \
        tag, lexer_token_clone(tok), {.tag = (struct tag){__VA_ARGS__}}})
