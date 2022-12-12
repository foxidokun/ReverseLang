#include <assert.h>
#include <cstdio>
#include "../lib/log.h"
#include "lexer.h"
#include "syntax_parser.h"

// Program        ::= PROG_BEG (SubProgram | Func)* PROG_END
// Func           ::= R_BRACKET OPEN_BLOCK Subprogram CLOSE_BLOCK L_BRACKET (NAME (SEP NAME)) NAME FN
// SubProgram     ::= (FlowBlock)+
// FlowBlock      ::= IfBlock | WhileBlock | OPEN_BLOCK Body CLOSE_BLOCK | Body
// WhileBlock     ::= OPEN_BLOCK Body CLOSE_BLOCK L_BRACKET Expression R_BRACKET WHILE
// IfBlock        ::= (OPEN_BLOCK Body CLOSE_BLOCK ELSE) OPEN_BLOCK Body CLOSE_BLOCK L_BRACKET Expression R_BRACKET IF
// Body           ::= (Line)+
// Line           ::= Expression (= NAME LET) BREAK
// Expression     ::= Expresssion = NAME | Expression PRINT | CompOperand (<=> CompOperand)
// CompOperand    ::= AddOperand  ([+-] AddOperand)* 
// AddOperand     ::= MulOperand  ([/ *] MulOperand )*
// MulOperand     ::= GeneralOperand ([^]  GeneralOperand)*
// GeneralOperand ::= L_BRACKET Expression R_BRACKET | Quant
// Quant          ::= VAR | VAL | INPUT | L_BRACKET (Expression (SEM Expression)) R_BRACKET NAME

using tree::node_type_t;
using tree::op_t;

// -------------------------------------------------------------------------------------------------

#define PREPARE()                       \
    assert ( input_token != nullptr);   \
    assert (*input_token != nullptr);   \
                                        \
    token_t *token      = *input_token; \
    tree::node_t *node  = nullptr;      \
    

#define SUCCESS()               \
{                               \
    *input_token = token;       \
    printf ("success in %s with last node: ", __func__);\
    program::print_token_func (token, stdout);          \
    printf ("\n");              \
    return node;                \
}

#define TRY(expr)               \
{                               \
    if ((expr) == nullptr)      \
    {                           \
        del_node (node);        \
        return nullptr;         \
    }                           \
}


#define EXPECT(expr)            \
{                               \
    if (!(expr))                \
    {                           \
        LOG (log::INF, "Syntax error on line %d: expected " #expr, token->line) \
        program::print_token_func (token, stdout);                              \
        printf ("\n");          \
        del_node (node);        \
        return nullptr;         \
    }                           \
}

#define CHECK(expr)     \
{                       \
    EXPECT (expr);      \
    token++;            \
}

#define isTYPE(expected_type) (token->type == token::type_t::expected_type)

#define isKEYWORD(expected_kw) (isTYPE(KEYWORD) && token->keyword == token::keyword::expected_kw)
#define isOP(expected_op)      (isTYPE(  OP   ) && token->op      == token::op::expected_op)

#define isNEXT_KEYWORD(expected_kw) ((token+1)->type    == token::type_t::KEYWORD && \
                                     (token+1)->keyword == token::keyword::expected_kw)
// -------------------------------------------------------------------------------------------------

static tree::node_t *GetFunc           (token_t **input_token);
static tree::node_t *GetSubProgram     (token_t **input_token);
static tree::node_t *GetFlowBlock      (token_t **input_token);
static tree::node_t *GetWhileBlock     (token_t **input_token);
static tree::node_t *GetIfBlock        (token_t **input_token);
static tree::node_t *GetBody           (token_t **input_token);
static tree::node_t *GetLine           (token_t **input_token);
static tree::node_t *GetExpression     (token_t **input_token);
static tree::node_t *GetCompOperand    (token_t **input_token);
static tree::node_t *GetAddOperand     (token_t **input_token);
static tree::node_t *GetMulOperand     (token_t **input_token);
static tree::node_t *GetGeneralOperand (token_t **input_token);
static tree::node_t *GetQuant          (token_t **input_token);

// -------------------------------------------------------------------------------------------------

tree::node_t *GetProgram (token_t *token)
{
    assert (token != nullptr && "invalid pointer");

    tree::node_t *node     = nullptr;
    tree::node_t *node_rhs = nullptr;

    while (true)
    {
        if ((node_rhs = GetSubProgram (&token)) == nullptr)
        {
            if ((node_rhs = GetFunc (&token)) == nullptr)
            {
                break;
            }
        }

        assert (node_rhs != nullptr && "Unexpected magic");

        node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_rhs);
    }

    CHECK (isKEYWORD (PROG_END));

    return node;
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetFunc (token_t **input_token)
{
    PREPARE();
    tree::node_t* arg_node = nullptr;

    CHECK (isKEYWORD (FN));
    EXPECT (isTYPE(NAME));
    int func_name = token->name;
    token++;

    CHECK (isKEYWORD (L_BRACKET));

    if (isTYPE (NAME)) {
        arg_node = tree::new_node (node_type_t::VAR, token->name);
        token++;

        while (isKEYWORD (SEP)) {
            token++;
            arg_node = tree::new_node (node_type_t::FICTIOUS, 0, arg_node, tree::new_node (node_type_t::VAR, token->name));
            token++;
        }
    }

    CHECK (isKEYWORD(R_BRACKET));
    CHECK (isKEYWORD (OPEN_BLOCK));
    node = GetSubProgram (&token);
    CHECK (isKEYWORD (CLOSE_BLOCK));

    node = tree::new_node (node_type_t::FUNC_DEF, func_name, arg_node, node);

    SUCCESS();
}

static tree::node_t *GetSubProgram (token_t **input_token)
{
    PREPARE();
    tree::node_t *node_rhs = nullptr;

    TRY (node = GetFlowBlock (&token));

    while ((node_rhs = GetFlowBlock (&token)) != nullptr)
    {
        node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_rhs);
    }

    SUCCESS ();
}

static tree::node_t *GetFlowBlock (token_t **input_token)
{
    PREPARE();

    if ((node = GetIfBlock (&token)) == nullptr) {
        if ((node = GetWhileBlock (&token)) == nullptr) {
            if (isKEYWORD (OPEN_BLOCK)) {
                token++;
                TRY (node = GetBody (&token));
                CHECK (isKEYWORD(CLOSE_BLOCK));
            } else {
                TRY (node = GetBody (&token));
            }
        }
    }

    SUCCESS ();
}

static tree::node_t *GetWhileBlock (token_t **input_token)
{
    PREPARE();
    tree::node_t *cond = nullptr;

    CHECK (isKEYWORD (WHILE));
    CHECK (isKEYWORD (L_BRACKET));

    TRY (cond = GetExpression (&token));
    
    CHECK (isKEYWORD (R_BRACKET));
    CHECK (isKEYWORD (OPEN_BLOCK));

    TRY (node = GetBody (&token));

    CHECK (isKEYWORD (CLOSE_BLOCK));

    SUCCESS();
}

static tree::node_t *GetIfBlock (token_t **input_token)
{
    PREPARE ();
    tree::node_t *cond     = nullptr;
    tree::node_t *node_rhs = nullptr;

    CHECK (isKEYWORD (IF));
    CHECK (isKEYWORD (L_BRACKET));

    TRY (cond = GetExpression (&token));

    CHECK (isKEYWORD (R_BRACKET));
    CHECK (isKEYWORD (OPEN_BLOCK));

    TRY (node = GetBody (&token));

    CHECK (isKEYWORD (CLOSE_BLOCK));

    if (isKEYWORD (ELSE))
    {
        token++;

        CHECK (isKEYWORD (OPEN_BLOCK));

        TRY (node_rhs = GetBody (&token));

        CHECK (isKEYWORD (CLOSE_BLOCK));

        node = tree::new_node (node_type_t::ELSE, 0, node, node_rhs);
    }

    node = new_node (node_type_t::IF, 0, cond, node);

    SUCCESS ();
}

static tree::node_t *GetBody (token_t **input_token)
{
    PREPARE ();
    tree::node_t *next_line = nullptr;

    TRY (node = GetLine (&token));

    while ((next_line = GetLine (&token)) != nullptr)
    {
        node = tree::new_node (node_type_t::FICTIOUS, 0, node, next_line);
    }

    SUCCESS();
}

static tree::node_t *GetLine (token_t **input_token)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;

    if (isKEYWORD (LET))
    {
        token++;

        EXPECT (isTYPE (NAME));
        node = tree::new_node (node_type_t::VAR_DEF, token->name);
        EXPECT (isNEXT_KEYWORD (ASSIG));
    }

    TRY (node_rhs = GetExpression (&token));
    node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_rhs);

    CHECK (isKEYWORD (BREAK));
    SUCCESS ();
}

static tree::node_t *GetExpression (token_t **input_token)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;

    if (isTYPE (NAME) && isNEXT_KEYWORD (ASSIG))
    {
        node = tree::new_node (tree::node_type_t::VAR, token->name);
        token++;

        assert (isKEYWORD (ASSIG) && "Unexpected magic: broken if condition");
        token++;

        TRY (node_rhs = GetExpression (&token));

        node = tree::new_node (node_type_t::OP, op_t::ASSIG, node, node_rhs);
    }
    else if (isKEYWORD (PRINT))
    {
        token++;

        TRY (node = GetExpression (&token));

        node = tree::new_node (node_type_t::OP, op_t::OUTPUT, nullptr, node);
    }
    else
    {
        TRY (node = GetCompOperand (&token));

        EXPECT (isKEYWORD (GE) || isKEYWORD (LE) || isKEYWORD (GT) || isKEYWORD (LT));
        token::keyword comp_op = token->keyword;

        
    }

    SUCCESS();
}

static tree::node_t *GetCompOperand (token_t **input_token)
{
    PREPARE();
    op_t op = tree::op_t::INPUT; // Poison value
    tree::node_t *node_rhs = nullptr;

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

        CHECK (isKEYWORD (R_BRACKET));
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

    if (isTYPE(NAME) && !isNEXT_KEYWORD (L_BRACKET))
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
    else if (isTYPE (NAME))
    {
        int name = token->name;
        tree::node_t *node_rhs = nullptr;
        token++;

        CHECK (isKEYWORD (L_BRACKET));
        TRY (node = GetExpression (&token));

        while (isKEYWORD (SEP))
        {
            token++;

            TRY (node_rhs = GetExpression (&token));
            node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_rhs);
        }

        CHECK (isKEYWORD (R_BRACKET));

        node = tree::new_node (node_type_t::FUNC_CALL, name, nullptr, node);
        SUCCESS ();
    }
    else if (isKEYWORD (INPUT))
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::INPUT);
        SUCCESS ();
    }
    
    EXPECT (0 && "Invalid Quant");
    return nullptr;
}