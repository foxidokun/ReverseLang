#include <cassert>
#include <cstdio>
#include <string.h>
#include "../lib/file.h"
#include "../lib/log.h"
#include "../lib/common.h"
#include "lexer.h"
#include "syntax_parser.h"
#include "frontend.h"
#include "codegen.h"

// -------------------------------------------------------------------------------------------------
static int  direct_frontend (const file_t *input_file, FILE *output_file);
static int reverse_frontend (const file_t *input_file, FILE *output_file);
// -------------------------------------------------------------------------------------------------

#define ERR_CASE(cond, fmt, ...)                    \
{                                                   \
    if (cond) {                                     \
        fprintf (stderr, fmt "\n", ##__VA_ARGS__);  \
        return ERROR;                               \
    }                                               \
}

// -------------------------------------------------------------------------------------------------

int main (int argc, const char* argv[])
{
    if ((argc != 3 && argc != 4) || strcmp (argv[1], "-h") == 0)
    {
        fprintf (stderr, "Usage: ./front (-r) <input file> <output file>\n");
        fprintf (stderr, "      -r for reverse codegen from ast dump\n");
        return ERROR;
    }

    int flag_cnt = 0;
    if (strcmp (argv[1], "-r") == 0) {
        flag_cnt = 1;
    }

    file_t input_file = open_ro_file (argv[1+flag_cnt]);
    ERR_CASE (input_file.content == nullptr, "Failed to open file %s", argv[1]);

    FILE *output_file = fopen (argv[2+flag_cnt], "w");
    ERR_CASE (output_file == nullptr, "Failed to open file %s", argv[2]);

    int res = 15; // random poison value

    if (flag_cnt == 0) {
        res = direct_frontend  (&input_file, output_file);
    } else {
        res = reverse_frontend (&input_file, output_file);
    }

    fclose (output_file);
    unmap_ro_file (input_file);
    return res;
}

// -------------------------------------------------------------------------------------------------

static int direct_frontend (const file_t *input_file, FILE *output_file)
{
    assert (input_file  != nullptr && "invalid pointer");
    assert (output_file != nullptr && "invalid pointer");

    program_t prog = {};
    program::ctor (&prog);

    if (program::tokenize (input_file->content, input_file->size, &prog) != 0) {
        ERR_CASE (true, "Failed to tokenise input file");
    }

    if (program::parse_into_ast (&prog) == ERROR) {
        program::dtor (&prog);
        ERR_CASE (true, "Failed to parse input file into AST");
    }

    program::save_ast (&prog, output_file);

    tree::del_node (prog.ast);
    program::dtor (&prog);

    return 0;
}

static int reverse_frontend (const file_t *input_file, FILE *output_file)
{
    assert (input_file  != nullptr && "invalid pointer");
    assert (output_file != nullptr && "invalid pointer");
    
    program_t prog = {};
    program::ctor (&prog);

    ERR_CASE (program::load_ast (&prog, input_file->content) == ERROR, 
                                                                    "Failed to load AST from dump");

    tree::graph_dump (prog.ast, "");

    program::codegen (&prog, output_file);

    tree::del_node (prog.ast);
    program::dtor (&prog);

    return 0;
}