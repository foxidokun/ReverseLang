#include <cassert>
#include <cstdio>
#include <cctype>
#include "../lib/common.h"
#include "frontend.h"
#include "lexer.h"

// -------------------------------------------------------------------------------------------------

const int MAX_NAME_LEN = 64;

// -------------------------------------------------------------------------------------------------

void program::save_ast (program_t *prog, FILE *stream)
{
    assert (prog != nullptr && "invalid pointer");

    program::save_names (prog, stream);
    fprintf (stream, "\n");
    tree::save_tree (prog->ast, stream);
}

// -------------------------------------------------------------------------------------------------

#define GET_VALUE(fmt, ...)                             \
{                                                       \
    sscanf (dump, fmt "%n", ##__VA_ARGS__, &_read_len); \
    dump += _read_len;                                  \
}

#define SKIP_SPACES()       \
{                           \
    while (isspace(*dump))  \
    {                       \
        dump++;             \
    }                       \
}

int program::load_ast (program_t *prog, const char *dump)
{
    assert (prog != nullptr);
    assert (dump != nullptr);

    int cnt                 = -420;
    int _read_len           = -420;
    char name[MAX_NAME_LEN] = "";

    GET_VALUE ("%d", &cnt);
    SKIP_SPACES ();

    for (int i = 0; i < cnt; ++i) {
        GET_VALUE ("%s", name);
        SKIP_SPACES ();
        nametable::insert_name (&prog->var_names, name);
    }

    GET_VALUE ("%d", &cnt);
    SKIP_SPACES ();

    for (int i = 0; i < cnt; ++i) {
        GET_VALUE ("%s", name);
        SKIP_SPACES ();
        nametable::insert_name (&prog->func_names, name);
    }

    prog->ast = tree::load_tree (dump);

    if (prog->ast == nullptr) return ERROR;
    else                      return 0;
}
