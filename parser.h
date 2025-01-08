#pragma once

#include "ast.h"
#include "lexer.h"
#include "uthash.h"

typedef struct parser parser;

typedef void (*parser_error_callback)(token tok, char const *NONNULL fmt,
                                      va_list arg);
typedef void (*parser_warning_callback)(token tok, char const *NONNULL fmt,
                                        va_list arg);

typedef expr *NULLABLE (*prefix_parse_fn)(parser *NONNULL p);
typedef expr *NULLABLE (*infix_parse_fn)(parser *NONNULL p, expr *NONNULL expr);

struct parser {
    lexer *NONNULL lexer;
    token          cur_token, peek_token;

    struct prefix_parse_fn_entry {
        token_kind            key;
        prefix_parse_fn NONNULL fn;
        UT_hash_handle        hh;
    } *NONNULL prefix_parse_fns;
    struct infix_parse_fn_entry {
        token_kind           key;
        infix_parse_fn NONNULL fn;
        UT_hash_handle       hh;
    } *NONNULL                      infix_parse_fns;

    u32                             errors, warnings;
    parser_error_callback NONNULL   ec;
    parser_warning_callback NONNULL wc;
};

program *NONNULL parse_program(parser *NONNULL p);

parser *NONNULL  parser_new(lexer *NONNULL l);
parser *NONNULL  parser_new_ex(lexer *NONNULL                   l,
                               parser_error_callback NULLABLE   ec,
                               parser_warning_callback NULLABLE wc);
// Frees the parser and its data, but not the lexer.
void parser_free(parser *NONNULL p);
