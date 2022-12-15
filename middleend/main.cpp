#include <cassert>
#include <cstdio>
#include <string.h>
#include "optimizer.h"
#include "../lib/file.h"
#include "../lib/common.h"

// -------------------------------------------------------------------------------------------------

#define ERR_CASE(cond, fmt, ...)                    \
{                                                   \
    if (cond) {                                     \
        fprintf (stderr, fmt "\n", ##__VA_ARGS__);  \
        tree::del_node (ast);                       \
        return ERROR;                               \
    }                                               \
}

// -------------------------------------------------------------------------------------------------

int main (int argc, const char *argv[])
{
    tree::node_t *ast = nullptr;

    ERR_CASE (argc != 3 || (strcmp (argv[1], "-h") == 0), "Usage: ./back <input ast file> <output asm file>");
    
    const file_t src = open_ro_file (argv[1]);
    ERR_CASE (src.content == nullptr, "Failed to open file %s", argv[1]);

    const char *tree_section = src.content;
    while (*tree_section != '{') tree_section++;

    ast = tree::load_tree (tree_section);
    ERR_CASE (ast == nullptr, "Failed to load AST tree");

    FILE *output_file = fopen (argv[2], "w");
    ERR_CASE (output_file == nullptr, "Failed to open file %s", argv[2]);

    optimize (ast);

    fwrite (src.content, sizeof (char), (size_t) (tree_section - src.content), output_file);
    tree::save_tree (ast, output_file);

    unmap_ro_file (src);
    fclose (output_file);
    tree::del_node (ast);
}