#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include "../lib/tree.h"

struct name_entry_t
{
    char *name;
    bool is_var;
    bool is_func;
};

struct nametable_t
{
    name_entry_t *names;
    unsigned int capacity;
    unsigned int size;
    unsigned int func_name_cnt;
    unsigned int var_name_cnt;
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
        LET,              //let
        ASSIG,            // =
        BREAK,            // ;
        PROG_BEG,         // ~nya~
        PROG_END,         // ~nya~
        PRINT,            // __builtin_print__
        INPUT,            // __builtin_input__
        SQRT,             // __builtin_sqrt__
        L_BRACKET,        // (
        R_BRACKET,        // )
        OPEN_BLOCK,       // {
        CLOSE_BLOCK,      // }
        FUNC_OPEN_BLOCK,  // {
        FUNC_CLOSE_BLOCK, // }
        RETURN,           // return
        SEP,              // ,
        IF,               // if
        ELSE,             // else
        WHILE,            // while
        FN,               // fn
        EQ,               // ==
        GE,               // >
        LE,               // <
        GT,               // >=
        LT,               // <=
        NEQ,              // !=
        NOT,              // !
        AND,              // &&
        OR,               // ||
    };

    enum class op
    {
        ADD,
        SUB,
        MUL,
        DIV,
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

    int line;
};

struct program_t
{
    token_t *tokens;
    size_t size;
    size_t capacity;

    nametable_t  names;
    tree::node_t *ast;

    int line;
};

namespace program
{
    void ctor (program_t *program);
    void dtor (program_t *program);

    void dump_tokens (program_t *program, FILE *stream);

    void save_names (program_t *program, FILE *stream);

    int tokenize (const char *const str_beg, size_t size, program_t *program);
    void print_token_func (token_t *token, FILE *stream);
}

#endif