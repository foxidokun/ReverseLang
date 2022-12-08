#include <assert.h>
#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <string.h>
#include "../lib/common.h"
#include "../lib/log.h"

#include "lexer.h"

// -------------------------------------------------------------------------------------------------
// TODO
// -------------------------------------------------------------------------------------------------
// 1. Валидация имен переменных, известны ли такие в данный момент. Можно как список глобальных + список локальных
// 2. Валидация кол-ва переменных у функции
// -------------------------------------------------------------------------------------------------

const int MAX_NAME_LEN = 128;

const int DEFAULT_NAMETABLE_SIZE = 64;
const int DEFAULT_TOKEN_COUNT    = 32;

// -------------------------------------------------------------------------------------------------

static bool tokenize_keyword (const char **str, program_t *program);
static bool tokenize_name    (const char **str, program_t *program);
static bool tokenize_val     (const char **str, program_t *program);
static bool tokenize_op      (const char **str, program_t *program);

static void realloc_tokens (program_t *program);

static void print_token_func (token_t *token, FILE *stream);

// -------------------------------------------------------------------------------------------------


#define SKIP_SPACES()           \
{                               \
    while (isspace(*str))       \
    {                           \
        if (*str == '\n')       \
        {                       \
            program->line++;    \
        }                       \
        str++;                  \
    }                           \
}

#define ERR_CASE(cond)                                                          \
{                                                                               \
    if (cond)                                                                   \
    {                                                                           \
        FILE *__log_stream_48de = get_log_stream();                             \
        LOG (log::ERR, "Bad token at line %d after token", program->line+1);    \
        fprintf (__log_stream_48de, "\t--> ");                                  \
        print_token_func (&program->tokens[program->size], __log_stream_48de);  \
        fprintf (__log_stream_48de, " <--\n");                                  \
        return ERROR;                                                           \
    }                                                                           \
}

// -------------------------------------------------------------------------------------------------
// PROGRAM NAMESPACE
// -------------------------------------------------------------------------------------------------

void program::ctor (program_t *program)
{
    assert (program != nullptr && "invalid pointer");

    program->tokens   = (token_t *) calloc(DEFAULT_TOKEN_COUNT, sizeof (token_t));
    program->size     = 0;
    program->capacity = DEFAULT_TOKEN_COUNT;
 
    nametable::ctor (&program->names);
}

void program::dtor (program_t *program)
{
    assert (program != nullptr && "invalid pointer");
    
    free (program->tokens);
    nametable::dtor (&program->names);
}

// -------------------------------------------------------------------------------------------------

int program::tokenize (const char *const str_beg, size_t size, program_t *program)
{
    assert (str_beg != nullptr && "invalid pointer");

    const char *str = str_beg;

    SKIP_SPACES ();

    while (str - str_beg < size)
    {
        if (isalpha (*str) || (*str == '=' || *str == '(' || *str == ')' || *str == '{' || *str == '}' || *str == ';' || *str == ',' || *str == '_' || *str == '~'))
        {
            if ( !tokenize_keyword(&str, program) )
            {
                ERR_CASE( !tokenize_name (&str, program) );
            }
            else 
            {
                ERR_CASE ( !(isspace(*str) || *str == ';'));
            }
        }
        else if (isdigit (*str))
        {
            ERR_CASE( !tokenize_val (&str, program) );
        }
        else 
        {
            ERR_CASE( !tokenize_op (&str, program) );
        }

        SKIP_SPACES ();
    }

    return 0;
}

// -------------------------------------------------------------------------------------------------

void program::dump_tokens (program_t *program, FILE *stream)
{
    assert (stream  != nullptr && "invalid pointer");
    assert (program != nullptr && "invalid pointer");

    printf ("\n// TOKENS_DUMP //\n\n");

    for (size_t i = 0; i < program->size; ++i)
    {
        printf ("[%zu] \t ", i);
        print_token_func (&program->tokens[i], stream);
        putc ('\n', stream);
    }
}

// -------------------------------------------------------------------------------------------------
// NAMETABLE NAMESPACE
// -------------------------------------------------------------------------------------------------

void nametable::ctor (nametable_t *nametable)
{
    assert (nametable != nullptr && "invalid pointer");

    nametable->names = (char **) calloc (sizeof (char *), DEFAULT_NAMETABLE_SIZE);

    nametable->size     = 0;
    nametable->capacity = DEFAULT_NAMETABLE_SIZE;
}

void nametable::dtor (nametable_t *nametable)
{
    assert (nametable != nullptr && "invalid pointer");

    for (size_t i = 0; i < nametable->size; ++i)
    {
        free (nametable->names[i]);
    }

    free (nametable->names);

    nametable->size     = 0;
    nametable->capacity = 0;
}

int nametable::insert_name (nametable_t *nametable, const char *name)
{
    assert (name      != nullptr && "invalid pointer");
    assert (nametable != nullptr && "invalid pointer");
    assert (nametable->names != nullptr && "invalid nametable");

    for (unsigned int i = 0; i < nametable->size; ++i)
    {
        if (strcmp (name, nametable->names[i]) == 0)
        {
            return (int) i;
        }
    }

    if (nametable->size == nametable->capacity)
    {
        nametable->names    = (char **) realloc (nametable->names, 2 * nametable->capacity * sizeof (char *));
        nametable->capacity = 2 * nametable->capacity;
    }

    nametable->names[nametable->size] = strdup (name);

    return (int) nametable->size++;
}


// -------------------------------------------------------------------------------------------------
// PRIVATE SECTION
// -------------------------------------------------------------------------------------------------

#undef  ERR_CASE
#define ERR_CASE(cond)  \
{                       \
    if (cond)           \
    {                   \
        return false;   \
    }                   \
}

#define SUCCESS()                                       \
{                                                       \
    program->size++;                                    \
    if (program->size == program->capacity) {           \
        realloc_tokens(program);                        \
    }                                                   \
    *input_str = str;                                   \
    return true;                                        \
}

// -------------------------------------------------------------------------------------------------

#define TRY_KEYWORD(type, text)                 \
if (strncmp (str, text, sizeof(text) - 1) == 0) \
{                                               \
    token->keyword = token::keyword::type;       \
    str += sizeof(text)-1;                      \
} else 

static bool tokenize_keyword (const char **input_str, program_t *program)
{
    assert (input_str     != nullptr && "invalid pointer");
    assert (*input_str    != nullptr && "invalid pointer");
    assert (program       != nullptr && "invalid pointer");

    const char *str = *input_str;
    token_t *token  = program->tokens + program->size;
    token->type     = token::type_t::KEYWORD;
    token->line     = program->line;

    TRY_KEYWORD (LET,          "let")
    TRY_KEYWORD (EQ,           "==")
    TRY_KEYWORD (ASSIG,        "=")
    TRY_KEYWORD (BREAK,        ";")
    TRY_KEYWORD (PROG_END,     "~nya~")
    TRY_KEYWORD (PRINT,        "__builtin_print__")
    TRY_KEYWORD (INPUT,        "__builtin_input__")
    TRY_KEYWORD (L_BRACKET,    "(")
    TRY_KEYWORD (R_BRACKET,    ")")
    TRY_KEYWORD (OPEN_BLOCK,   "{")
    TRY_KEYWORD (CLOSE_BLOCK,  "}")
    TRY_KEYWORD (SEP,          ",")
    TRY_KEYWORD (IF,           "if")
    TRY_KEYWORD (ELSE,         "else")
    TRY_KEYWORD (WHILE,        "while")
    TRY_KEYWORD (FN,           "fn")
    TRY_KEYWORD (GE,           "ge")
    TRY_KEYWORD (LE,           "le")
    TRY_KEYWORD (GT,           "gt")
    TRY_KEYWORD (LT,           "lt")
    TRY_KEYWORD (NEQ,          "neq")
    TRY_KEYWORD (NOT,          "not")
    TRY_KEYWORD (AND,          "and")
    TRY_KEYWORD (OR,           "or")

    /*else*/ {
        return false;
    }

    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

static bool tokenize_name (const char **input_str, program_t *program)
{
    assert (input_str  != nullptr && "invalid pointer");
    assert (*input_str != nullptr && "invalid pointer");
    assert (program    != nullptr && "invalid pointer");

    const char *str = *input_str;
    token_t *token  = program->tokens + program->size;
    token->type     = token::type_t::NAME;
    token->line     = program->line;

    char name_buf[MAX_NAME_LEN] = "";
    int len = 0;

    ERR_CASE (sscanf (str, "%[a-zA-Z0-9_]%n", name_buf, &len) != 1);
    str += len;
    token->name = nametable::insert_name (&program->names, name_buf);

    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

static bool tokenize_val (const char **input_str, program_t *program)
{
    assert (input_str     != nullptr && "invalid pointer");
    assert (*input_str    != nullptr && "invalid pointer");
    assert (program       != nullptr && "invalid pointer");

    const char *str = *input_str;
    token_t *token  = program->tokens + program->size;
    token->type     = token::type_t::VAL;
    token->line     = program->line;

    int val_buf = 0;
    int len     = 0;
    
    ERR_CASE (sscanf (str, "%d%n", &val_buf, &len) != 1);
    str += len;
    token->val = val_buf;

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static bool tokenize_op (const char **input_str, program_t *program)
{
    assert (input_str     != nullptr && "invalid pointer");
    assert (*input_str    != nullptr && "invalid pointer");
    assert (program       != nullptr && "invalid pointer");

    const char *str = *input_str;
    token_t *token  = program->tokens + program->size;
    token->type     = token::type_t::OP;
    token->line     = program->line;

    switch (*str)
    {
        case '+': token->op = token::op::ADD; break;
        case '-': token->op = token::op::SUB; break;
        case '*': token->op = token::op::MUL; break;
        case '/': token->op = token::op::DIV; break;
        case '^': token->op = token::op::POW; break;

        default:
            return false;
    }

    str++;

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static void realloc_tokens (program_t *program)
{
    assert (program != nullptr && "invalid pointer");

    program->tokens = (token_t *) realloc (program->tokens, 2 * program->capacity * sizeof (token_t));
}


// -------------------------------------------------------------------------------------------------

#define EMIT(str, ...)                      \
{                                           \
    fprintf (stream, str, ##__VA_ARGS__);   \
    return;                                 \
}

static void print_token_func (token_t *token, FILE *stream)
{
    assert (token  != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    switch (token->type)
    {
        case token::type_t::KEYWORD:
            switch (token->keyword)
            {
                case token::keyword::LET:         EMIT ("KW: let");
                case token::keyword::ASSIG:       EMIT ("KW: :=");
                case token::keyword::BREAK:       EMIT ("KW: BREAK");
                case token::keyword::PROG_END:    EMIT ("KW: ~nya~");
                case token::keyword::PRINT:       EMIT ("KW: //print//");
                case token::keyword::INPUT:       EMIT ("KW: //input//");
                case token::keyword::L_BRACKET:   EMIT ("KW: (");
                case token::keyword::R_BRACKET:   EMIT ("KW: )");
                case token::keyword::IF:          EMIT ("KW: if");
                case token::keyword::EQ:          EMIT ("KW: ==");
                case token::keyword::OPEN_BLOCK:  EMIT ("KW: {");
                case token::keyword::CLOSE_BLOCK: EMIT ("KW: }");
                case token::keyword::SEP:         EMIT ("KW: SEP");
                case token::keyword::ELSE:        EMIT ("KW: ELSE");
                case token::keyword::WHILE:       EMIT ("KW: WHILE");
                case token::keyword::FN:          EMIT ("KW: FN");
                case token::keyword::GE:          EMIT ("KW: >=");
                case token::keyword::LE:          EMIT ("KW: <=");
                case token::keyword::GT:          EMIT ("KW: > ");
                case token::keyword::LT:          EMIT ("KW: < ");
                case token::keyword::NEQ:         EMIT ("KW: !=");
                case token::keyword::NOT:         EMIT ("KW: NOT");
                case token::keyword::AND:         EMIT ("KW: &&");
                case token::keyword::OR:          EMIT ("KW: ||");

                default: assert (0 && "unknown keyword");
            }

        case token::type_t::OP:
            switch (token->op)
            {
                case token::op::ADD: EMIT ("OP: +");
                case token::op::SUB: EMIT ("OP: -");
                case token::op::MUL: EMIT ("OP: *");
                case token::op::DIV: EMIT ("OP: /");
                case token::op::POW: EMIT ("OP: pow");

                default: assert (0 && "unknown op");
            }

        case token::type_t::VAL : EMIT ("VAL: %d",    token->val );
        case token::type_t::NAME: EMIT ("NAME: #%d",  token->name);

        default: assert (0 && "unknown token type");
    }
}