#ifndef SYNTAX_PARSER_H
#define SYNTAX_PARSER_H

#include "../lib/tree.h"
#include "lexer.h"

tree::node_t *GetProgram (token_t *token);

#endif