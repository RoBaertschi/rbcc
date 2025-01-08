#include "parser.h"
#include <stdlib.h>
#include "ast.h"
#include "lexer.h"
#include "rbcc.h"
#include "uthash.h"

typedef enum precedence {
    PLOWEST,
} precedence;

static void default_error_callback(token tok, char const *fmt, va_list arg) {
    loc     loc = tok.loc;
    va_list arg2;
    va_copy(arg2, arg);

    int   buffer_size = vsnprintf(NULL, 0, fmt, arg) + 1;
    char *buffer      = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    fprintf(stdout, "%s[%d:%d] Parser Error %s\n", loc.file.data, loc.line,
            loc.column, buffer);
    free(buffer);

    va_end(arg2);
}

static void default_warning_callback(token tok, char const *fmt, va_list arg) {
    loc     loc = tok.loc;
    va_list arg2;
    va_copy(arg2, arg);

    int   buffer_size = vsnprintf(NULL, 0, fmt, arg) + 1;
    char *buffer      = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    fprintf(stdout, "%s[%d:%d] Parser Warning %s\n", loc.file.data, loc.line,
            loc.column, buffer);
    free(buffer);

    va_end(arg2);
}

static void PRINTF_FORMAT(3, 4)
    error(parser *NONNULL p, token token, char const *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    if (p->ec != NULL) {
        p->ec(token, fmt, arg);
    }
    p->errors += 1;

    va_end(arg);
}

static void PRINTF_FORMAT(3, 4)
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
    lexer_token_free(p->cur_token);
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
    }                                              \
    next_token(p);

#define expect_peek(k)                                                 \
    if (!tok_peek_is(p, k)) {                                          \
        error(p, p->peek_token, "expected peek token kind %s, got %s", \
              token_kind_str(k), token_kind_str(p->peek_token.kind));  \
        return NULL;                                                   \
    }                                                                  \
    next_token(p);

precedence get_precedence(token_kind kind) {
    switch (kind) {
        default:
            return PLOWEST;
    }
}

precedence peek_precedence(parser *NONNULL p) {
    return get_precedence(p->peek_token.kind);
}

precedence cur_precedence(parser *NONNULL p) {
    return get_precedence(p->cur_token.kind);
}

static expr *NULLABLE parse_constant(parser *p) {
    expect(TCONSTANT);

    return EXPR_NEW(expr_constant, p->cur_token, p->cur_token.data.constant);
}

static expr *NULLABLE parse_expr(parser *NONNULL p, precedence prec) {
    struct prefix_parse_fn_entry *entry;
    HASH_FIND(hh, p->prefix_parse_fns, &p->cur_token.kind, sizeof(token_kind),
              entry);
    if (entry == NULL) {
        error(p, p->cur_token, "could not find a prefix function for %s",
              token_kind_str(p->cur_token.kind));
        return NULL;
    }
    expr *left_expr = entry->fn(p);

    while (!tok_peek_is(p, TSEMICOLON) && prec < peek_precedence(p)) {
        struct infix_parse_fn_entry *infix;
        HASH_FIND(hh, p->infix_parse_fns, &p->peek_token.kind,
                  sizeof(token_kind), infix);
        if (infix == NULL) {
            return left_expr;
        }

        next_token(p);

        left_expr = infix->fn(p, left_expr);
    }

    return left_expr;
}

static stmt *NULLABLE parse_function(parser *NONNULL p) {
    expect(TFN);
    expect_peek(TIDENT);
    str_slice lit        = p->cur_token.literal;
    str       identifier = str_slice_clone(lit);
    expect_peek(TOPEN_PAREN);
    expect_peek(TCLOSE_PAREN);
    expect_peek(TEQUAL);
    next_token(p);
    expr *e = parse_expr(p, PLOWEST);
    expect_peek(TSEMICOLON);
    return STMT_NEW(stmt_function, identifier, e);
}

program *NONNULL parse_program(parser *p) {
    return program_new((program){
        .main_function = parse_function(p),
    });
}

void register_prefix_fn(parser *NONNULL parser, prefixParseFn fn,
                        token_kind kind) {
    struct prefix_parse_fn_entry *entry =
        xmalloc(sizeof(struct prefix_parse_fn_entry));
    *entry = (struct prefix_parse_fn_entry){
        .fn  = fn,
        .key = kind,
    };
    HASH_ADD(hh, parser->prefix_parse_fns, key, sizeof(token_kind), entry);
}

void register_infix_fn(parser *NONNULL parser, infixParseFn fn,
                       token_kind kind) {
    struct infix_parse_fn_entry *entry =
        xmalloc(sizeof(struct infix_parse_fn_entry));
    *entry = (struct infix_parse_fn_entry){
        .fn  = fn,
        .key = kind,
    };
    HASH_ADD(hh, parser->infix_parse_fns, key, sizeof(token_kind), entry);
}

parser *NONNULL parser_new(lexer *l) {
    return parser_new_ex(l, default_error_callback, default_warning_callback);
}

// Sepcify additional callbacks
parser *NONNULL parser_new_ex(lexer *l, parser_error_callback NULLABLE ec,
                              parser_warning_callback NULLABLE wc) {
    parser *p = xmalloc(sizeof(parser));
    *p        = (parser){
               .lexer            = l,

               .infix_parse_fns  = NULL,
               .prefix_parse_fns = NULL,

               .ec               = ec,
               .wc               = wc,
               .warnings         = 0,
               .errors           = 0,
    };

    register_prefix_fn(p, parse_constant, TCONSTANT);

    next_token(p);
    next_token(p);

    return p;
}
void parser_free(parser *NONNULL p) {

    {
        struct prefix_parse_fn_entry *el, *tmp;
        HASH_ITER(hh, p->prefix_parse_fns, el, tmp) {
            HASH_DEL(p->prefix_parse_fns, el);
            free(el);
        }
    }
    {
        struct infix_parse_fn_entry *el, *tmp;
        HASH_ITER(hh, p->infix_parse_fns, el, tmp) {
            HASH_DEL(p->prefix_parse_fns, el);
            free(el);
        }
    }
    lexer_token_free(p->cur_token);
    lexer_token_free(p->peek_token);
    free(p);
}
