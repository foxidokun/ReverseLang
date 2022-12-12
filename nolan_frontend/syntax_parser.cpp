#include <assert.h>
#include <cstdio>
#include "../lib/log.h"
#include "lexer.h"
#include "syntax_parser.h"

// Program        ::= PROG_BEG (SubProgram | Func)* PROG_END
// Func           ::= FUNC_OPEN_BLOCK Subprogram FUNC_CLOSE_BLOCK L_BRACKET (NAME (SEP NAME)) R_BRACKET NAME FN
// SubProgram     ::= (FlowBlock)+
// FlowBlock      ::= IfBlock | WhileBlock | OPEN_BLOCK Body CLOSE_BLOCK | Body
// WhileBlock     ::= OPEN_BLOCK Body CLOSE_BLOCK L_BRACKET Expression R_BRACKET WHILE
// IfBlock        ::= (OPEN_BLOCK Body CLOSE_BLOCK ELSE) OPEN_BLOCK Body CLOSE_BLOCK L_BRACKET Expression R_BRACKET IF
// Body           ::= (Line)+
// Line           ::= BREAK Expression RETURN | BREAK Expression (= NAME (LET))
// Expression     ::= PRINT Expression | CompOperand (<=> CompOperand) // TODO: Somehow reverse 'name = expr' and add it here
// CompOperand    ::= AddOperand  ([+-] AddOperand)* 
// AddOperand     ::= MulOperand  ([/ *] MulOperand )*
// MulOperand     ::= GeneralOperand ([^]  GeneralOperand)*
// GeneralOperand ::= Quant | L_BRACKET Expression R_BRACKET
// Quant          ::= VAR | VAL | INPUT | L_BRACKET (Expression (SEM Expression)) R_BRACKET NAME

using tree::node_type_t;
using tree::op_t;

// -------------------------------------------------------------------------------------------------

#define PREPARE()                       \
    assert ( input_token != nullptr);   \
    assert (*input_token != nullptr);   \
                                        \
    printf ("Called %s\n", __func__); \
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

#define CHECK_KEYWORD(kw) CHECK(isKEYWORD(kw))

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
    tree::node_t *node_next = nullptr;

    CHECK_KEYWORD (PROG_BEG);

    while (true)
    {
        if ((node_next = GetSubProgram (&token)) == nullptr)
        {
            if ((node_next = GetFunc (&token)) == nullptr)
            {
                break;
            }
        }

        assert (node_next != nullptr && "Unexpected magic");

        node = tree::new_node (node_type_t::FICTIOUS, 0, node_next, node);
    }

    CHECK_KEYWORD (PROG_END);

    return node;
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetFunc (token_t **input_token)
{
    PREPARE();
    tree::node_t* arg_node = nullptr;

    CHECK_KEYWORD (FUNC_OPEN_BLOCK);
    TRY (node = GetSubProgram (&token));
    CHECK_KEYWORD (FUNC_CLOSE_BLOCK);

    CHECK_KEYWORD (L_BRACKET);
    
    if (isTYPE (NAME))
    {
        arg_node = tree::new_node (node_type_t::VAR_DEF, token->name);
        token++;

        while (isKEYWORD (SEP))
        {
            token++;

            EXPECT (isTYPE (NAME));
            arg_node = tree::new_node (node_type_t::FICTIOUS, 0, 
                                        tree::new_node (node_type_t::VAR_DEF, token->name),
                                        arg_node);
            token++;
        }
    }
    
    CHECK_KEYWORD (R_BRACKET);
    EXPECT (isTYPE(NAME));
    node = tree::new_node (node_type_t::FUNC_DEF, token->name, arg_node, node);
    token++;
    CHECK_KEYWORD (FN);

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetSubProgram (token_t **input_token)
{
    PREPARE();
    tree::node_t *next_block = nullptr;

    TRY (node = GetFlowBlock (&token));

    while ((next_block = GetFlowBlock (&token)) != nullptr)
    {
        node = tree::new_node (node_type_t::FICTIOUS, 0, next_block, node);
    }

    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetFlowBlock (token_t **input_token)
{
    PREPARE();

    if ((node = GetIfBlock (&token)) == nullptr) {
        if ((node = GetWhileBlock (&token)) == nullptr) {
            if (isKEYWORD (OPEN_BLOCK)) {
                token++;
                TRY (node = GetBody (&token));
                CHECK_KEYWORD(CLOSE_BLOCK);
            } else {
                TRY (node = GetBody (&token));
            }
        }
    }

    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetWhileBlock (token_t **input_token)
{
    PREPARE();
    tree::node_t *cond = nullptr;

    CHECK_KEYWORD (OPEN_BLOCK);
    TRY (node = GetBody (&token));
    CHECK_KEYWORD (CLOSE_BLOCK);
    
    CHECK_KEYWORD (L_BRACKET);
    TRY(cond = GetBody (&token));
    CHECK_KEYWORD (R_BRACKET);

    CHECK_KEYWORD (WHILE);

    node = tree::new_node (node_type_t::WHILE, 0, cond, node);
    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetIfBlock (token_t **input_token)
{
    PREPARE ();
    tree::node_t *cond     = nullptr;
    tree::node_t *tmp_node = nullptr;

    CHECK_KEYWORD (OPEN_BLOCK);
    TRY (node = GetBody (&token));
    CHECK_KEYWORD (CLOSE_BLOCK);

    if (isKEYWORD (ELSE))
    {
        token++;

        CHECK_KEYWORD (OPEN_BLOCK);
        TRY (tmp_node = GetBody (&token));
        CHECK_KEYWORD (CLOSE_BLOCK);

        node = tree::new_node (node_type_t::ELSE, 0, tmp_node, node);
    }

    CHECK_KEYWORD (L_BRACKET);
    TRY (cond = GetExpression (&token));
    CHECK_KEYWORD (R_BRACKET);
    CHECK_KEYWORD (IF);

    node = tree::new_node (node_type_t::IF, 0, cond, node);
    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetBody (token_t **input_token)
{
    PREPARE ();
    tree::node_t *next_line = nullptr;

    TRY (node = GetLine (&token));

    while ((next_line = GetLine (&token)) != nullptr)
    {
        node = tree::new_node (node_type_t::FICTIOUS, 0, next_line, node);
    }

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetLine (token_t **input_token)
{
    PREPARE ();

    tree::node_t *var_def = nullptr;

    CHECK_KEYWORD (BREAK);
    TRY (node = GetExpression (&token));

    if (isKEYWORD (ASSIG))
    {
        token++;

        EXPECT (isTYPE (NAME));
        int var_name = token->name;
        node = tree::new_node (node_type_t::OP, op_t::ASSIG, tree::new_node(node_type_t::VAR, var_name), node);
        token++;

        if (isKEYWORD (LET))
        {
            token++;
            var_def = tree::new_node (node_type_t::VAR_DEF, var_name);
        }
    }
    else if (isKEYWORD (RETURN))
    {
        token++;
        
        node = tree::new_node (node_type_t::RETURN, 0, nullptr, node);
    }

    node = tree::new_node (node_type_t::FICTIOUS, 0, var_def, node);

    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

#define TRANSLATE_KEYWORD(op_in)  \
    case token::keyword::op_in:   \
        node = tree::new_node (tree::node_type_t::OP, tree::op_t::op_in, node_rhs, node); \
        break;                    \

static tree::node_t *GetExpression (token_t **input_token)
{
    PREPARE ();

    if (isKEYWORD (PRINT))
    {
        token++;

        TRY (node = GetExpression (&token));

        node = tree::new_node (node_type_t::OP, op_t::OUTPUT, nullptr, node);
    }
    else
    {
        TRY (node = GetCompOperand (&token));

        if (isKEYWORD (GE) || isKEYWORD (LE) || isKEYWORD (GT) || isKEYWORD (LT))
        {
            tree::node_t *node_rhs = nullptr;
            token::keyword comp_op = token->keyword;
            token++;

            TRY (node_rhs = GetCompOperand (&token));

            switch (comp_op) {
                TRANSLATE_KEYWORD (GE);
                TRANSLATE_KEYWORD (LE);
                TRANSLATE_KEYWORD (LT);
                TRANSLATE_KEYWORD (GT);
                
                default: assert (0 && "Logic error: incorrect if condition");
            }
        }
    }

    SUCCESS();
}

#undef TRANSLATE_KEYWORD
// -------------------------------------------------------------------------------------------------

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

        node = tree::new_node (node_type_t::OP, op, node_rhs, node);
    }

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

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

        node = tree::new_node (node_type_t::OP, op, node_rhs, node);
    }

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetMulOperand (token_t **input_token)
{
    PREPARE();

    tree::node_t *node_rhs = nullptr;

    TRY (node = GetGeneralOperand (&token));

    while (isOP (POW))
    {
        token++;
        TRY (node_rhs = GetGeneralOperand (&token))

        node = tree::new_node (node_type_t::OP, op_t::POW, node_rhs, node);
    }

    SUCCESS();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetGeneralOperand (token_t **input_token)
{
    PREPARE ();

    if ((node = GetQuant(&token)) != nullptr)
    {
        SUCCESS ();
    }

    CHECK_KEYWORD(L_BRACKET);
    TRY (node = GetExpression (&token));
    CHECK_KEYWORD (R_BRACKET);

    SUCCESS ();
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetQuant (token_t **input_token)
{
    PREPARE();

    if (isTYPE(NAME))
    {
        node = tree::new_node (tree::node_type_t::VAR, token->name);
        token++;
    }
    else if (isTYPE(VAL))
    {
        node = tree::new_node (tree::node_type_t::VAL, token->val);
        token++;
    }
    else if (isKEYWORD (L_BRACKET))
    {
        token++;
        tree::node_t *node_param = nullptr;

        TRY (node = GetExpression (&token));

        while (isKEYWORD (SEP))
        {
            token++;

            TRY (node_param = GetExpression (&token));
            node = tree::new_node (node_type_t::FICTIOUS, 0, node_param, node);
        }

        CHECK_KEYWORD (R_BRACKET);
        EXPECT (isTYPE (NAME));
        int name = token->name;
        token++;

        node = tree::new_node (node_type_t::FUNC_CALL, name, nullptr, node);
    }
    else if (isKEYWORD (INPUT))
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::INPUT);
    }
    else {
        EXPECT (0 && "Invalid Quant");
        return nullptr;
    }

    SUCCESS ();    
}