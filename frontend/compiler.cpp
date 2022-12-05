#include <assert.h>
#include "compiler.h"

#define EMIT(fmt, ...) fprintf (stream, fmt, ##__VA_ARGS);

void compile (tree::node_t *ast, FILE *stream)
{
    assert (stream != nullptr && "invalid pointer");
    assert (ast    != nullptr && "invalid tree");

    switch (ast->type)
    {
        case tree::node_type_t::NOT_SET:
        case tree::node_type_t::FICTIOUS:
        case tree::node_type_t::VAL:
        case tree::node_type_t::VAR:

        case tree::node_type_t::VAR_DEF:
            register_var (ast->data);
            break;

        case tree::node_type_t::IF:
        case tree::node_type_t::ELSE:
        case tree::node_type_t::WHILE:
        case tree::node_type_t::OP:
        case tree::node_type_t::FUNC_DEF:
        case tree::node_type_t::FUNC_CALL:
        case tree::node_type_t::RETURN:
            assert (0 && "Not Implemented");
    }
}