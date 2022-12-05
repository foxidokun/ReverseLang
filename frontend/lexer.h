#ifndef LEXER_H
#define LEXER_H

#include"stdlib.h"
#include"stdio.h"

struct nametable_t
{
    char **names;
    unsigned int capacity;
    unsigned int size;
};

namespace nametable {
    void ctor (nametable_t *nametable);
    void dtor (nametable_t *nametable);

    int insert_name (nametable_t *nametable, const char *name);
}

namespace token {
    enum class type_t 
    {
        KEYWORD,
        OP,
        VAL,
        NAME,
    };

    enum class keyword
    {
        LET,        //let
        EQ,         // =
        BREAK,      // ;
        PROG_END,   // ~nya~
        PRINT,      // __builtin_print__
        L_BRACKET,  // (
        R_BRACKET   // )
    };

    enum class op
    {
        ADD,
        SUB,
        MUL,
        DIV,
        POW
    };
}

struct token_t {
    token::type_t type;
    union {
        int            name;
        int            val;
        token::keyword keyword;
        token::op      op;
    };
};

struct program_t
{
    token_t *tokens;
    size_t size;
    size_t capacity;

    nametable_t  names;
};

namespace program
{
    void ctor (program_t *program);
    void dtor (program_t *program);

    void dump_tokens (program_t *program, FILE *stream);

    int tokenize (const char *const str_beg, size_t size, program_t *program);
}

#endif