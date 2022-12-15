#include <assert.h>
#include <cstdio>
#include <string.h>
#include "compiler.h"
#include "../lib/common.h"
#include "../lib/file.h"

#define ERR_CASE(cond, fmt, ...)                    \
{                                                   \
    if (cond) {                                     \
        fprintf (stderr, fmt "\n", ##__VA_ARGS__);  \
        return ERROR;                               \
    }                                               \
}

int main (int argc, const char *argv[])
{
    ERR_CASE (argc != 3 || (argc == 2 && strcmp (argv[1], "-h")),
                                                "Usage: ./back <input ast file> <output asm file>");

    const file_t src = open_ro_file (argv[1]);
    ERR_CASE (src.content == nullptr, "Failed to open file %s", argv[1]);

    const char *tree_section = src.content;
    while (*tree_section != '{') tree_section++;

    tree::node_t *ast = tree::load_tree (tree_section);
    ERR_CASE (ast == nullptr, "Failed to load AST tree");
    unmap_ro_file (src);

    FILE *output_file = fopen (argv[2], "w");
    ERR_CASE (output_file == nullptr, "Failed to open file %s", argv[2]);

    ERR_CASE (compiler::compile (ast, output_file), "Failed to compile, see logs");

    tree::del_node (ast);
}