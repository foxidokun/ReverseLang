#include <assert.h>
#include "../lib/file.h"
#include "lexer.h"
#include "syntax_parser.h"
#include "compiler.h"
#include "frontend.h"

int main ()
{
    file_t input = open_ro_file ("input.txt");

    program_t prog = {};
    program::ctor (&prog);

    if (program::tokenize (input.content, input.size, &prog) != 0)
    {
        assert (0 && "Fail tokenization");
    }

    program::dump_tokens (&prog, stdout);

    tree::node_t *head = GetProgram (&prog);

    assert (head != nullptr && "Invalid head");

    tree::graph_dump (head, "Hueta");

    save_ast (&prog, head, fopen ("ast.txt", "w"));

    compiler::compile (head, fopen ("test.asm", "w"));

    tree::del_node (head);
    program::dtor (&prog);
}