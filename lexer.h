#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include "rbcc.h"
#include "utf8proc.h"

#define TOKENS        \
    _X(EOF)           \
    _X(INVALID)       \
    _X(IDENT)         \
    /* Literals */    \
    _X(CONSTANT)      \
    _X(STRING)        \
    /* Punctiation */ \
    _X(OPEN_PAREN)    \
    _X(CLOSE_PAREN)   \
    _X(EQUAL)         \
    _X(SEMICOLON)     \
    /* Keywords */    \
    _X(FN) // Function Keywords

typedef enum token_kind {
#define _X(tok) T##tok,
    TOKENS
#undef _X
        TMAX_TOKEN,
} token_kind;

extern char const *const token_kind_strs[];

char const              *token_kind_str(token_kind kind);

typedef struct loc {
    u32 pos;
    u32 line;
    u32 column;
    str file;
} loc;

typedef union token_data {
    i64 constant;
} token_data;

typedef struct token {
    token_kind kind;
    loc        loc;
    str_slice  literal;
    token_data data;
} token;

typedef struct tokens {
    token *data;
    size_t len;
} tokens;

typedef void (*error_callback)(loc loc, char const *fmt, va_list arg);
typedef struct lexer {
    u32              pos;
    u32              read_pos;
    u32              pos_since_line;
    u32              line;

    utf8proc_int32_t ch;
    str              input;
    str              file;

    u32              errors;
    error_callback   ec;
} lexer;

token  lexer_scan_token(lexer *l);

lexer *lexer_new(str input);
lexer *lexer_new_ex(str input, error_callback ec);
void   lexer_free(lexer *l);
