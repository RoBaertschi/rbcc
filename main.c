#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "emit_ir.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "rbcc.h"
#include "targets/targets.h"

#define BUFFER_SIZE 1024

typedef enum arg_kind {
    ARG_PRINT_AST,
    ARG_PRINT_IR,
    ARG_PRINT_ALL,
} arg_kind;

typedef struct arg {
    arg_kind type;
    str      string;
} arg;

str get_program_name(char *argv) {
    if (*argv != 0) {
        size_t len = strlen(argv);
        return (str){.data = (u8 *)argv, .len = len};
    } else {
        return S("rbc");
    }
}

void print_help(int exit_code, str program_name) {
    printf("%s [--print] FILE\n", program_name.data);
    printf("  --help       # Print this help\n");
    printf("  --print=all  # Print the ast and ir to stdout\n");
    printf("  --print=ast  # Print the ast to stdout\n");
    printf("  --print=ir   # Print the ir to stdout\n");
    printf("  --no-emit    # Do not emit any assembly or executables\n");
    exit(exit_code);
}

int main(int argc, char **argv) {
    (void)argc;
    arg_kind print_mode      = ARG_PRINT_ALL;

    str      program_name    = get_program_name(argv[0]);
    bool     found_input_file, found_output_file = false;
    str      input_file, output_file;
    bool     emit = true;
    argv += 1; // skip the first argument
    while (*argv != NULL) {
        switch (**argv) {
            case '-':
                if (str_eq((str){.data = (u8 *)*argv, .len = strlen(*argv)},
                           S("--print=ast"))) {
                    print_mode = ARG_PRINT_AST;
                } else if (str_eq(
                               (str){.data = (u8 *)*argv, .len = strlen(*argv)},
                               S("--print=ir"))) {
                    print_mode = ARG_PRINT_IR;
                } else if (str_eq(
                               (str){.data = (u8 *)*argv, .len = strlen(*argv)},
                               S("--print=all"))) {
                    print_mode = ARG_PRINT_ALL;
                } else if (str_eq(
                               (str){.data = (u8 *)*argv, .len = strlen(*argv)},
                               S("--no-emit"))) {
                    emit = false;
                } else if (str_eq(
                               (str){.data = (u8 *)*argv, .len = strlen(*argv)},
                               S("--help"))) {
                    print_help(0, program_name);
                } else {
                    printf("unknown option \"%s\"\n", *argv);
                    print_help(1, program_name);
                }
                break;
            default:
                if (!found_input_file) {
                    input_file =
                        (str){.data = (u8 *)*argv, .len = strlen(*argv)};
                    found_input_file = true;
                } else {
                    printf("unknown argument \"%s\"\n", *argv);
                    print_help(1, program_name);
                }
        }
        argv += 1;
    }

    if (!found_input_file) {
        printf("no input file specified \"%s\"\n", *argv);
        print_help(1, program_name);
    }

    FILE *file = fopen((char *)input_file.data, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        return 1;
    }

    u8    *string  = NULL;
    size_t str_len = 0;
    while (true) {
        char   buffer[BUFFER_SIZE];
        size_t read =
            fread(buffer, sizeof(buffer) / BUFFER_SIZE, BUFFER_SIZE, file);

        if (string == NULL) {
            string  = xmalloc(read);
            str_len = read;
            memcpy(string, buffer, read);
        } else {
            string = realloc(string, str_len + read);
            CHECK_ALLOC(string);
            memcpy(string + str_len, buffer, read), str_len += read;
        }

        if (read < BUFFER_SIZE) {
            break;
        }
    }
    fclose(file);

    string = realloc(string, str_len + 1);
    CHECK_ALLOC(string);
    string[str_len] = 0;

    str input       = {
              .data = string,
              .len  = str_len,
    };

    lexer   *l       = lexer_new(input);

    parser  *p       = parser_new(l);

    program *program = parse_program(p);
    if (print_mode != ARG_PRINT_IR) {
        program_print(program);
        printf("\n");
    }

    ir_program ir_program = ir_emit_program(program);

    if (print_mode != ARG_PRINT_AST) {
        ir_program_print(&ir_program);
    }

    if (emit) {
        str with_suffix = file_name_with_suffix(input_file, S("fasm"));
        code_gen((char const *)with_suffix.data, TARGET_X86_64_LINUX,
                 ir_program);
        str_free(with_suffix);
    }

    ir_program_free(&ir_program);

    program_free(program);

    parser_free(p);
    lexer_free(l);

    free(string);
}
