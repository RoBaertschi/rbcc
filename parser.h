#pragma once

#include "ast.h"
#include "lexer.h"

typedef void (*parser_error_callback)(token tok, char const *fmt, va_list arg);
typedef void (*parser_warning_callback)(token tok, char const *fmt,
                                        va_list arg);

typedef struct parser {
    lexer                  *lexer;
    token                   cur_token, peek_token;

    u32                     errors, warnings;
    parser_error_callback   ec;
    parser_warning_callback wc;
} parser;

program *parse_program(parser *p);

parser  *parser_new(lexer *l);
parser  *parser_new_ex(lexer *l, parser_error_callback ec,
                       parser_warning_callback wc);
// Frees the parser and its data, but not the lexer.
void parser_free(parser *p);
