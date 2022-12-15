#ifndef CODEGEN_H
#define CODEGEN_H
#include "lexer.h"

namespace program {
    void codegen (program_t *program, FILE *stream);
}

#endif