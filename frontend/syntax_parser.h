#ifndef SYNTAX_PARSER_H
#define SYNTAX_PARSER_H

#include "../lib/tree.h"
#include "lexer.h"

namespace program 
{
    int parse_into_ast (program_t *prog);
}

#endif