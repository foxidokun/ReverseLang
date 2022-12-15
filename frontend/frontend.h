#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdio.h>
#include "lexer.h"
#include "../lib/tree.h"

namespace program {
    void save_ast (program_t *prog, FILE *stream);

    int load_ast (program_t *prog, const char *dump);
}

#endif