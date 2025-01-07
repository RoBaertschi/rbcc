#pragma once

#include "rbcc.h"
typedef struct ir_program     ir_program;

typedef struct ir_function    ir_function;

typedef struct ir_instruction ir_instruction;

struct ir_program {};

struct ir_function {
    str name;
};

struct ir_instruction {
    enum ir_instruction_kind { INST_RET } kind;
    union {} data;
};
