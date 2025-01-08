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

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
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
        program_print(program);
        printf("\n");

        ir_program ir_program = ir_emit_program(program);

        ir_program_print(&ir_program);

        ir_program_free(&ir_program);

        program_free(program);

        parser_free(p);
        lexer_free(l);

        free(string);
    } else {
        printf("Invalid usage of this command. Please provide a file to use.");
    }
}
