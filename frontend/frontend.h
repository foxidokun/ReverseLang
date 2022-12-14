#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdio.h>
#include "lexer.h"
#include "../lib/tree.h"

void save_ast (program_t *prog, tree::node_t *ast, FILE *stream);

#endif