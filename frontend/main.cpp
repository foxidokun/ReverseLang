#include <assert.h>
#include "../lib/file.h"
#include "../lib/log.h"
#include "../lib/common.h"
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

    if (program::parse_into_ast (&prog) == ERROR) {
        LOG (log::ERR, "Failed to parse input file");
        return ERROR;
    }

    save_ast (&prog, prog.ast, fopen ("ast.txt", "w"));

    tree::del_node (prog.ast);
    program::dtor (&prog);
}