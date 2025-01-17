#if defined (__linux__) || defined (__unix__)
#define _POSIX_C_SOURCE 200809L
#endif
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

#include "subprocess.h"
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
    printf("%s FILE\n", program_name.data);
    printf("  --help       # Print this help\n");
    printf("  --print=all  # Print the ast and ir to stdout\n");
    printf("  --print=ast  # Print the ast to stdout\n");
    printf("  --print=ir   # Print the ir to stdout\n");
    printf("  --no-emit    # Do not emit any assembly or executables\n");
    printf("  -o FILE      # Specify the output file for the executable\n");
    exit(exit_code);
}

str fread_full(FILE *file) {
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

    string = realloc(string, str_len + 1);
    CHECK_ALLOC(string);
    string[str_len] = 0;

    return (str){
        .data = string,
        .len  = str_len,
    };
}

bool launch_program(char const *const *args, str *in_out, str *in_err) {

    struct subprocess_s s;
    if (subprocess_create(args,
                          subprocess_option_inherit_environment |
                              subprocess_option_search_user_path,
                          &s) != 0) {
        return false;
    }
    int result = 0;
    if (subprocess_join(&s, &result) != 0) {
        subprocess_destroy(&s);
        return false;
    }

    FILE *out = subprocess_stdout(&s), *err = subprocess_stderr(&s);

    *in_out = fread_full(out);
    *in_err = fread_full(err);

    if (result != 0) {
        subprocess_destroy(&s);
        return false;
    }

    subprocess_destroy(&s);
    return true;
}

int main(int argc, char **argv) {
    (void)argc;
    arg_kind print_mode       = ARG_PRINT_ALL;

    str      program_name     = get_program_name(argv[0]);
    bool     found_input_file = false, found_output_file = false;
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
                               S("-o"))) {
                    argv += 1;
                    if (*argv == NULL) {
                        printf("expected file, got no more arguments\n");
                        print_help(1, program_name);
                    } else {
                        output_file =
                            (str){.data = (u8 *)*argv, .len = strlen(*argv)};
                        found_output_file = true;
                    }
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

    if (!found_output_file && emit) {
        printf("no output file specified");
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
        str fasm_file = file_name_with_suffix(input_file, S("fasm"));
        str o_file    = file_name_with_suffix(input_file, S("o"));

        code_gen((char const *)fasm_file.data, TARGET_X86_64_LINUX, ir_program);

        str out, err;
        if (launch_program((char const *const[]){"fasm", (char *)fasm_file.data,
                                                 (char *)o_file.data, NULL},
                           &out, &err)) {
            if (!launch_program(
                    (char const *const[]){"gcc", (char *)o_file.data, "-o",
                                          (char *)output_file.data, NULL},
                    &out, &err)) {
                printf("failed to run gcc\n%s\n%s\n", out.data, err.data);
            }
        } else {
            printf("failed to run fasm\n%s\n%s\n", out.data, err.data);
        }
        remove((char *)fasm_file.data);
        remove((char *)o_file.data);
        str_free(fasm_file);
        str_free(o_file);
    }

    ir_program_free(&ir_program);

    program_free(program);

    parser_free(p);
    lexer_free(l);

    free(string);
}
