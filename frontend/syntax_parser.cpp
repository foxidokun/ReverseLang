#include <assert.h>
#include <cstdio>
#include "lexer.h"
#include "syntax_parser.h"

// Program        ::= Action (KEYWORD::BREAK Action)* PROG_END
// Action         ::= ((KEYWORD::LET) NAME KEYWORD::EQ | KEYWORD::PRINT) Expression
// Expression     ::= AddOperand  ([+-] AddOperand  )*
// AddOperand     ::= MulOperand  ([/ *] MulOperand )*
// MulOperand     ::= GeneralOperand ([^]  GeneralOperand)*
// GeneralOperand ::= L_BRACKET Expression R_BRACKET | Quant
// Quant          ::= (VAR | VAL)

using tree::node_type_t;
using tree::op_t;

#include "lexer.h"

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
                case token::keyword::LET:       EMIT ("KW: let");
                case token::keyword::EQ:        EMIT ("KW: =");
                case token::keyword::BREAK:     EMIT ("KW: BREAK");
                case token::keyword::PROG_END:  EMIT ("KW: ~nya~");
                case token::keyword::PRINT:     EMIT ("KW: //print//");
                case token::keyword::L_BRACKET: EMIT ("KW: (");
                case token::keyword::R_BRACKET: EMIT ("KW: )");

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

// -------------------------------------------------------------------------------------------------

#define PREPARE()                       \
    assert ( input_token != nullptr);   \
    assert (*input_token != nullptr);   \
                                        \
    token_t *token      = *input_token; \
    tree::node_t *node  = nullptr;       \
    printf ("Func start: %s with token ", __func__);\
    print_token_func (token, stdout);\
    printf ("\n");\

#define SUCCESS()               \
{                               \
    *input_token = token;       \
    printf ("Func success: %s with token ", __func__);\
    print_token_func (token, stdout);\
    printf ("\n");\
    return node;                \
}

#define TRY(expr)               \
{                               \
    if ((expr) == nullptr)      \
    {                           \
        printf (#expr " CHECK FAILED\n");\
        del_node (node);        \
        return nullptr;         \
    }                           \
}

#define EXPECT(expr)            \
{                               \
    if (!(expr))                \
    {                           \
        printf (#expr " EXPECTATION FAILED, token: "); \
        print_token_func (token, stdout);\
        printf ("\n");\
        del_node (node);        \
        return nullptr;         \
    }                           \
}

#define isTYPE(expected_type) (token->type == token::type_t::expected_type)

#define isKEYWORD(expected_kw) (isTYPE(KEYWORD) && token->keyword == token::keyword::expected_kw)
#define isOP(expected_op)      (isTYPE(  OP   ) && token->op      == token::op::expected_op)

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetAction          (token_t **input_token);
static tree::node_t *GetExpression      (token_t **input_token);
static tree::node_t *GetAddOperand      (token_t **input_token);
static tree::node_t *GetMulOperand      (token_t **input_token);
static tree::node_t *GetGeneralOperand  (token_t **input_token);
static tree::node_t *GetQuant           (token_t **input_token);

// -------------------------------------------------------------------------------------------------

tree::node_t *GetProgram (token_t *token)
{
    assert (token != nullptr && "invalid pointer");
    tree::node_t *node     = nullptr;
    tree::node_t *node_rhs = nullptr;

    TRY (node = GetAction (&token));

    while (isKEYWORD (BREAK))
    {
        token++;

        if (!isKEYWORD (PROG_END)) {
            TRY (node_rhs = GetAction (&token));
            node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_rhs);
        } else {
            return node;
        }
    }

    return nullptr;
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetAction (token_t **input_token)
{
    PREPARE();

    if (isKEYWORD (LET))
    {
        token++;
        EXPECT (isTYPE(NAME)); // No token++ for next if

        node = tree::new_node (node_type_t::VAR_DEF, token->name);
    }

    if (isTYPE (NAME))
    {
        int name =  token->name;
        token++;

        EXPECT (isKEYWORD (EQ));
        token++;

        tree::node_t *node_rhs = nullptr;
        TRY (node_rhs = GetExpression (&token));

        node_rhs = tree::new_node (node_type_t::OP, op_t::ASSIG,
                                    tree::new_node (node_type_t::VAR, name),
                                    node_rhs
                                    );

        node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_rhs);
    }
    else
    {
        EXPECT (isKEYWORD (PRINT));
        token++;

        TRY (node = GetExpression (&token));

        node = tree::new_node (node_type_t::OP, op_t::OUTPUT, nullptr, node);
    }

    SUCCESS ();
}

static tree::node_t *GetExpression (token_t **input_token)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;
    op_t op = tree::op_t::INPUT; // Poison value

    TRY (node = GetAddOperand (&token));

    while (isOP(ADD) || isOP(SUB))
    {
        op = isOP (ADD) ? op_t::ADD : op_t::SUB;
        token++;

        TRY (node_rhs = GetAddOperand (&token));

        node = tree::new_node (node_type_t::OP, op, node, node_rhs);
    }

    SUCCESS();
}


static tree::node_t *GetAddOperand (token_t **input_token)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;
    op_t op = tree::op_t::INPUT; // Poison value

    TRY (node = GetMulOperand (&token));

    while (isOP(MUL) || isOP(DIV))
    {
        op = isOP (MUL) ? op_t::MUL : op_t::DIV;
        token++;

        TRY (node_rhs = GetMulOperand (&token));

        node = tree::new_node (node_type_t::OP, op, node, node_rhs);
    }

    SUCCESS();
}


static tree::node_t *GetMulOperand (token_t **input_token)
{
    PREPARE();

    tree::node_t *node_rhs = nullptr;

    TRY (node = GetGeneralOperand (&token));

    while (isOP (POW))
    {
        token++;
        TRY (node_rhs = GetGeneralOperand (&token))

        node = tree::new_node (node_type_t::OP, op_t::POW, node, node_rhs);
    }

    SUCCESS();
}


static tree::node_t *GetGeneralOperand (token_t **input_token)
{
    PREPARE ();

    if (isKEYWORD (L_BRACKET))
    {
        token++;
        assert (node != nullptr);

        EXPECT (isKEYWORD (R_BRACKET));
        token++;
    }
    else 
    {
        TRY (node = GetQuant(&token));
    }

    SUCCESS ();
}

static tree::node_t *GetQuant (token_t **input_token)
{
    PREPARE();

    if (isTYPE(NAME))
    {
        node = tree::new_node (tree::node_type_t::VAR, token->name);
        token++;
        SUCCESS ();
    }
    else if (isTYPE(VAL))
    {
        node = tree::new_node (tree::node_type_t::VAL, token->val);
        token++;
        SUCCESS ();
    }
    
    return nullptr;
}