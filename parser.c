#include "parser.h"
#include <math.h>
#include "ast.h"
#include "lexer.h"
#include "rbcc.h"

void default_error_callback(token tok, char const *fmt, va_list arg) {
    loc     loc = tok.loc;
    va_list arg2;
    va_copy(arg2, arg);

    int   buffer_size = vsnprintf(NULL, 0, fmt, arg) + 1;
    char *buffer      = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    fprintf(stdout, "%s[%d:%d] Parser Error %s", loc.file.data, loc.line,
            loc.column, buffer);
    free(buffer);

    va_end(arg2);
}

void default_warning_callback(token tok, char const *fmt, va_list arg) {
    loc     loc = tok.loc;
    va_list arg2;
    va_copy(arg2, arg);

    int   buffer_size = vsnprintf(NULL, 0, fmt, arg) + 1;
    char *buffer      = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    fprintf(stdout, "%s[%d:%d] Parser Warning %s", loc.file.data, loc.line,
            loc.column, buffer);
    free(buffer);

    va_end(arg2);
}

void PRINTF_FORMAT(3, 4)
    error(parser *NONNULL p, token token, char const *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    if (p->ec != NULL) {
        p->ec(token, fmt, arg);
    }
    p->errors += 1;

    va_end(arg);
}

void PRINTF_FORMAT(3, 4)
    warning(parser *NONNULL p, token token, char const *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    if (p->wc != NULL) {
        p->wc(token, fmt, arg);
    }
    p->warnings += 1;

    va_end(arg);
}

static void next_token(parser *p) {
    p->cur_token  = p->peek_token;
    p->peek_token = lexer_scan_token(p->lexer);
}

static bool tok_is(parser *NONNULL p, token_kind kind) {
    return p->cur_token.kind == kind;
}

static bool tok_peek_is(parser *NONNULL p, token_kind kind) {
    return p->peek_token.kind == kind;
}

#define expect_msg(kind, msg, ...)                \
    if (!tok_is(p, kind)) {                       \
        error(p, p->cur_token, msg, __VA_ARGS__); \
        return NULL;                              \
    }

#define expect(k)                                                    \
    if (!tok_is(p, k)) {                                             \
        error(p, p->cur_token, "expected token kind %s, got %s",     \
              token_kind_str(k), token_kind_str(p->cur_token.kind)); \
        return NULL;                                                 \
    }

#define expect_peek_msg(kind, msg, ...)            \
    if (!tok_peek_is(p, kind)) {                   \
        error(p, p->peek_token, msg, __VA_ARGS__); \
        return NULL;                               \
    }                                                                  \
    next_token(p);

#define expect_peek(k)                                                 \
    if (!tok_peek_is(p, k)) {                                          \
        error(p, p->peek_token, "expected peek token kind %s, got %s", \
              token_kind_str(k), token_kind_str(p->peek_token.kind));  \
        return NULL;                                                   \
    }                                                                  \
    next_token(p);

static expr *NULLABLE parse_expr(parser *p) { return NULL; }

static stmt *NULLABLE parse_function(parser *p) {
    expect(TFN);
    expect_peek(TIDENT);
    str_slice lit        = p->cur_token.literal;
    str       identifier = str_slice_clone(lit);
    expect_peek(TOPEN_PAREN);
    expect_peek(TCLOSE_PAREN);
    expect_peek(TEQUAL);
    expr *e = parse_expr(p);
    expect_peek(TSEMICOLON);
    return STMT_NEW(stmt_function, identifier, e);
}

program *NONNULL parse_program(parser *p) {
    program *prog       = xmalloc(sizeof(program));
    prog->main_function = parse_function(p);
    return prog;
}

parser *NONNULL parser_new(lexer *l) {
    return parser_new_ex(l, default_error_callback, default_warning_callback);
}

// Sepcify additional callbacks
parser *NONNULL parser_new_ex(lexer *l, parser_error_callback ec,
                              parser_warning_callback wc) {
    parser *p = xmalloc(sizeof(parser));
    *p        = (parser){
               .lexer    = l,
               .ec       = ec,
               .wc       = wc,
               .warnings = 0,
               .errors   = 0,
    };

    next_token(p);
    next_token(p);

    return p;
}
void parser_free(parser *p) { free(p); }
