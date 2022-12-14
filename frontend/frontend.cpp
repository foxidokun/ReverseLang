#include <cassert>
#include "frontend.h"
#include "lexer.h"

void save_ast (program_t *prog, tree::node_t *ast, FILE *stream)
{
    assert (prog != nullptr && "invalid pointer");
    assert (ast  != nullptr && "invalid pointer");

    program::save_names (prog, stream);
    fprintf (stream, "\n");
    tree::save_tree (ast, stream);
}
