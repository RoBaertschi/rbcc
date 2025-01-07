#include "lexer.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbcc.h"
#include "utf8proc.h"
#include "uthash.h"

char const *const token_kind_strs[] = {
#define _X(name) [T##name] = #name,
    TOKENS
#undef _X
};

static_assert((sizeof(token_kind_strs) / sizeof(char const *const)) ==
                  TMAX_TOKEN,
              "The token_kind_strs array has to have the same amount of "
              "entries as there are tokens.");

char const *token_kind_str(token_kind kind) {
    if (kind >= TMAX_TOKEN || kind < 0) {
        return "UNKNOWN TOKEN";
    }
    return token_kind_strs[kind];
}

// Keywords map

typedef struct keyword {
    char const    *key;
    token_kind     value;
    UT_hash_handle hh;
} keyword;

static keyword *keywords = NULL;

#define KEYWORD(name, v)                         \
    do {                                         \
        keyword *kw  = xmalloc(sizeof(keyword)); \
        char    *key = xmalloc(sizeof(name));    \
        memcpy(key, name, sizeof(name));         \
        kw->key   = key;                         \
        kw->value = v;                           \
        HASH_ADD_STR(keywords, key, kw);         \
    } while (0)

static void init_keywords(void) {
    if (keywords == NULL) {
        KEYWORD("fn", TFN);
    }
}

static void uninit_keywords(void) {
    if (keywords != NULL) {
        keyword *el, *tmp;
        HASH_ITER(hh, keywords, el, tmp) {
            HASH_DEL(keywords, el);
            // We cast the const away
            free((char *)el->key);
            free(el);
        }
    }
}

static void default_error_callback(loc loc, char const *fmt, va_list arg) {
    va_list arg2;
    va_copy(arg2, arg);

    int   buffer_size = vsnprintf(NULL, 0, fmt, arg) + 1;
    char *buffer      = xmalloc(buffer_size);
    vsnprintf(buffer, buffer_size, fmt, arg2);
    fprintf(stdout, "%s[%d:%d] Lexer Error %s", loc.file.data, loc.line,
            loc.column, buffer);
    free(buffer);

    va_end(arg2);
}

static bool is_letter(utf8proc_uint32_t ch) {
    const utf8proc_property_t *prop = utf8proc_get_property(ch);
    switch (prop->category) {
        case UTF8PROC_CATEGORY_LT:
        case UTF8PROC_CATEGORY_LL:
        case UTF8PROC_CATEGORY_LU:
        case UTF8PROC_CATEGORY_LM:
        case UTF8PROC_CATEGORY_LO:
            return true;
    }
    return false;
}

// Returns -1 on failure
static i32 number_value(utf8proc_uint32_t ch) {
    switch (ch) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return ch - '0';
    }
    return -1;
}

static bool is_number(utf8proc_uint32_t ch) {
    switch (ch) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return true;
    }
    return false;
}

loc pos_to_loc(lexer *l, u32 pos) {
    return (loc){.file   = str_clone(l->file),
                 .line   = l->line,
                 .column = pos - l->pos_since_line + 1,
                 .pos    = pos};
}

void PRINTF_FORMAT(2, 3) error(lexer *l, char const *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);

    loc loc = pos_to_loc(l, l->pos);
    if (l->ec != NULL) {
        l->ec(loc, fmt, arg);
    }
    l->errors += 1;

    va_end(arg);
}

void read_ch(lexer *l) {
    if (l->read_pos < l->input.len) {
        l->pos = l->read_pos;
        if (l->ch == '\n') {
            l->pos_since_line = l->pos;
            l->line += 1;
        }
        utf8proc_ssize_t read = utf8proc_iterate(l->input.data + l->pos,
                                                 l->input.len - l->pos, &l->ch);
        if (read < 0) {
            switch (read) {
                case UTF8PROC_ERROR_INVALIDUTF8:
                    error(l, "invalid utf8 character");
                    break;
                case UTF8PROC_ERROR_OVERFLOW:
                    error(l, "input string is to big");
                    break;
                case UTF8PROC_ERROR_NOMEM:
                    CHECK_ALLOC(NULL);
                    error(l,
                          "CRITICAL ERROR: RAN OUT OF MEMORY AND DIDN'T ABORT");
                    break;
                case UTF8PROC_ERROR_NOTASSIGNED:
                default:
                    error(l, "unknown utf8 error");
                    break;
            }
        }

        if (l->ch == 0xfeff && l->pos > 0) {
            error(l, "illegal bom");
        }

        // Debug
        /*u8 string[5];*/
        /*size_t w = utf8proc_encode_char(l->ch, string);*/
        /*string[w] = 0;*/
        /*printf("lex: %s\n", string);*/

        l->read_pos += read;
    } else {
        l->pos = l->input.len;
        if (l->ch == '\n') {
            l->pos_since_line = l->pos;
            l->line += 1;
        }
        l->ch = -1;
    }
}

static str_slice scan_ident(lexer *l) {
    u32 old_pos = l->pos;
    while (is_letter(l->ch) || is_number(l->ch)) {
        read_ch(l);
    }

    // dhapd
    // ^
    // 01234
    // old_pos = 0
    // dhapd
    //      ^
    // old_pos = 0
    // l->pos  = 5
    // .data = l->input.data + old_pos == l->input.data
    // .len  = l->pos - old_pos == 5 - 0 == 5

    return (str_slice){.data = l->input.data + old_pos,
                       .len  = l->pos - old_pos};
}

// l.ch == '"'
static str_slice scan_string(lexer *l) {
    u32 old_pos = l->pos;
    read_ch(l); // skip '"'
    while (l->ch != '"' && l->ch != -1) {
        read_ch(l);
    }
    read_ch(l); // eat '"'

    return (str_slice){.data = l->input.data + old_pos,
                       .len  = l->pos - old_pos};
}
typedef struct scan_constant_result {
    i64       value;
    str_slice literal;
} scan_constant_result;
static scan_constant_result scan_constant(lexer *l) {
    u32  old_pos = l->pos;
    bool minus   = false;
    if (l->ch == '-') {
        minus = true;
        read_ch(l);
    }
    i64    value = 0;
    size_t i     = 1;

    // 324
    // ^
    // value = 3, i = 10
    // 324
    //  ^
    // value = 23, i = 100
    // 324
    //   ^
    // value = 423, i = 1000
    //
    //
    // 423
    //
    // 423 % 100 = 23
    // (423 - 23) / 100 = 4

    while (is_number(l->ch)) {
        value += number_value(l->ch) * i;
        i *= 10;
        read_ch(l);
    }

    i64    reverse    = 0;
    size_t multiplier = 1;
    while (i % 10 == 0) {
        i /= 10; // 100, 10, 1
        i64 mod = value % i;
        reverse += ((value - mod) / i) * multiplier;
        multiplier *= 10;
    }

    value = minus ? -reverse : reverse;
    return (scan_constant_result){
        .value   = value,
        .literal = (str_slice){.data = l->input.data + old_pos,
                               .len  = l->pos - old_pos}
    };
}

static void skip_whitespace(lexer *l) {
    while (l->ch == '\n' || l->ch == '\t' || l->ch == ' ') {
        read_ch(l);
    }
}

token lexer_scan_token(lexer *l) {
    skip_whitespace(l);

    token_kind kind    = TINVALID;
    loc        loc     = pos_to_loc(l, l->pos);
    str_slice  literal = {.data = l->input.data + l->pos, .len = 1};
    token_data data    = {0};

    if (is_letter(l->ch)) {
        kind    = TIDENT;
        literal = scan_ident(l);
        keyword *out;
        HASH_FIND(hh, keywords, literal.data, literal.len, out);
        if (out != NULL) {
            kind = out->value;
        }
    } else if (l->ch == '"') {
        kind    = TSTRING;
        literal = scan_string(l);
    } else if (is_number(l->ch)) {
        scan_constant_result result = scan_constant(l);
        literal                     = result.literal;
        data                        = (token_data){.constant = result.value};
        kind                        = TCONSTANT;
    } else {
        switch (l->ch) {
            case -1:
                kind = TEOF;
                break;
            case '(':
                kind = TOPEN_PAREN;
                break;
            case ')':
                kind = TCLOSE_PAREN;
                break;
            case '=':
                kind = TEQUAL;
                break;
            case ';':
                kind = TSEMICOLON;
                break;
        }
        read_ch(l);
    }

    return (token){
        .literal = literal,
        .kind    = kind,
        .data    = data,
        .loc     = loc,
    };
}

lexer *lexer_new(str input) {
    return lexer_new_ex(input, default_error_callback);
}

lexer *lexer_new_ex(str input, lexer_error_callback ec) {
    init_keywords();
    lexer *l = xmalloc(sizeof(struct lexer));

    *l       = (struct lexer){
              .pos            = 0,
              .read_pos       = 0,
              .pos_since_line = 0,
              .line           = 1,
              .input          = input,
              .errors         = 0,
              .ec             = ec,
    };

    read_ch(l);
    if (l->ch == 0xfeff) {
        read_ch(l);
    }

    return l;
}

void lexer_free(lexer *l) {
    free(l);
    uninit_keywords();
}
