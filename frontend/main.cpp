#include <assert.h>
#include "../lib/file.h"
#include "lexer.h"
#include "syntax_parser.h"

int main ()
{
    file_t input = open_ro_file ("input.txt");

    program_t prog = {};
    program::ctor (&prog);

    program::tokenize (input.content, input.size, &prog);

    program::dump_tokens (&prog, stdout);

    tree::node_t *head = GetProgram (prog.tokens);

    tree::graph_dump (head, "Hueta");

    tree::del_node (head);
    program::dtor (&prog);
}