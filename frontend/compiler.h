#ifndef COMPILER_H
#define COMPILER_H

#include "../lib/tree.h"

struct vars_t {
    int *name_indexes;

    unsigned int size;
    unsigned int capacity;
};

//TODO понадобится стек размеров фрейма
struct compiler_t
{
    vars_t vars;

    int cur_label_index;
};

namespace compiler
{
    void ctor (compiler_t *compiler);
    void dtor (compiler_t *compiler);

    void compile (tree::node_t *node, FILE *stream);
}

#endif