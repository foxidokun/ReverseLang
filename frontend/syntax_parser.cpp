#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include "../lib/log.h"
#include "../lib/common.h"
#include "lexer.h"
#include "syntax_parser.h"

// Program        ::= PROG_BEG (SubProgram | Func)* PROG_END
// Func           ::= L_BRACKET (NAME (SEP NAME)) R_BRACKET NAME FN FUNC_OPEN_BLOCK Subprogram FUNC_CLOSE_BLOCK
// SubProgram     ::= (FlowBlock)+
// FlowBlock      ::= IfBlock | WhileBlock | OPEN_BLOCK Body CLOSE_BLOCK | Body
// WhileBlock     ::= L_BRACKET Expression R_BRACKET WHILE OPEN_BLOCK Body CLOSE_BLOCK
// IfBlock        ::= L_BRACKET Expression R_BRACKET IF OPEN_BLOCK Body CLOSE_BLOCK (ELSE OPEN_BLOCK Body CLOSE_BLOCK) 
// Body           ::= (Line)+
// Line           ::= BREAK Expression RETURN | BREAK Expression (= NAME (LET))
// Expression     ::= OrOperand (|| OrOperand)+
// OrOperand      ::= AndOperand (&& AndOperand)+
// AndOperand     ::= CompOperand (<=> CompOperand)
// CompOperand    ::= AddOperand  ([+-] AddOperand)* 
// AddOperand     ::= MulOperand  ([/ *] MulOperand )*
// MulOperand     ::= GeneralOperand (NOT)
// GeneralOperand ::= Quant | L_BRACKET Expression R_BRACKET
// Quant          ::= VAR | VAL | INPUT | BuiltInFunc | L_BRACKET (Expression (SEM Expression)) R_BRACKET NAME
// BuiltInFunc    ::= L_BRACKET Expression R_BRACKET (PRINT|SQRT|SIN)

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
    printf ("success in %s with last node on line %d: ", __func__, token->line);\
    program::print_token_func (token, stdout);          \
    printf ("\n");              \
    return node;                \
}

#define TRY(expr)               \
{                               \
    if ((expr) == nullptr)      \
    {                           \
        del_node (node);        \
        EXTRA_CLEAR_ON_ERROR(); \
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
        EXTRA_CLEAR_ON_ERROR(); \
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

#define SET_NAME_TYPE(orig_name_index, new_name_index, type)                                \
{                                                                                           \
    new_name_index = nametable::insert_name (&prog->type##_names,                           \
                                                prog->all_names.names[orig_name_index]);    \
}

// -------------------------------------------------------------------------------------------------

static tree::node_t *GetProgram        (program_t *prog);
static tree::node_t *GetFunc           (token_t **input_token, program_t *prog);
static tree::node_t *GetSubProgram     (token_t **input_token, program_t *prog);
static tree::node_t *GetFlowBlock      (token_t **input_token, program_t *prog);
static tree::node_t *GetWhileBlock     (token_t **input_token, program_t *prog);
static tree::node_t *GetIfBlock        (token_t **input_token, program_t *prog);
static tree::node_t *GetBody           (token_t **input_token, program_t *prog);
static tree::node_t *GetLine           (token_t **input_token, program_t *prog);
static tree::node_t *GetExpression     (token_t **input_token, program_t *prog);
static tree::node_t *GetOrOperand      (token_t **input_token, program_t *prog);
static tree::node_t *GetAndOperand     (token_t **input_token, program_t *prog);
static tree::node_t *GetCompOperand    (token_t **input_token, program_t *prog);
static tree::node_t *GetAddOperand     (token_t **input_token, program_t *prog);
static tree::node_t *GetMulOperand     (token_t **input_token, program_t *prog);
static tree::node_t *GetGeneralOperand (token_t **input_token, program_t *prog);
static tree::node_t *GetQuant          (token_t **input_token, program_t *prog);
static tree::node_t *GetBuiltInFunc    (token_t **input_token, program_t *prog);

// -------------------------------------------------------------------------------------------------

int program::parse_into_ast (program_t *prog)
{
    assert (prog != nullptr && "invalid pointer");

    prog->ast = GetProgram (prog);

    if (prog->ast == nullptr) { return ERROR; }
    else                      { return 0;     }
}

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetProgram (program_t *prog)
{
    assert (prog != nullptr && "invalid pointer");

    token_t *token          = prog->tokens; 
    tree::node_t *node      = nullptr;
    tree::node_t *node_next = nullptr;

    CHECK_KEYWORD (PROG_BEG);

    while (true)
    {
        if ((node_next = GetSubProgram (&token, prog)) == nullptr)
        {
            if ((node_next = GetFunc (&token, prog)) == nullptr)
            {
                break;
            }
        }

        assert (node_next != nullptr && "Unexpected magic");

        node = tree::new_node (node_type_t::FICTIOUS, 0, node, node_next);
    }

    CHECK_KEYWORD (PROG_END);

    return node;
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() tree::del_node (arg_node);

static tree::node_t *GetFunc (token_t **input_token, program_t *prog)
{
    PREPARE();
    tree::node_t* arg_node = nullptr;
    int real_name = -1;

    CHECK_KEYWORD (L_BRACKET);
    
    if (isTYPE (NAME))
    {
        SET_NAME_TYPE (token->name, real_name, var);
        arg_node = tree::new_node (node_type_t::VAR, real_name);
        token++;

        while (isKEYWORD (SEP))
        {
            token++;

            EXPECT (isTYPE (NAME));
            SET_NAME_TYPE (token->name, real_name, var);

            arg_node = tree::new_node (node_type_t::FICTIOUS, 0, 
                                        tree::new_node (node_type_t::VAR, real_name),
                                        arg_node);
            token++;
        }
    }
    
    CHECK_KEYWORD (R_BRACKET);
    EXPECT (isTYPE(NAME));
    int func_name = token->name;
    SET_NAME_TYPE (token->name, func_name, func);
    token++;
    
    CHECK_KEYWORD (FN);

    CHECK_KEYWORD (FUNC_OPEN_BLOCK);
    TRY (node = GetSubProgram (&token, prog));
    CHECK_KEYWORD (FUNC_CLOSE_BLOCK);
    
    node = tree::new_node (node_type_t::FUNC_DEF, func_name, arg_node, node);
    
    SUCCESS();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetSubProgram (token_t **input_token, program_t *prog)
{
    PREPARE();
    tree::node_t *next_block = nullptr;

    TRY (node = GetFlowBlock (&token, prog));

    while ((next_block = GetFlowBlock (&token, prog)) != nullptr)
    {
        node = tree::new_node (node_type_t::FICTIOUS, 0, node, next_block);
    }

    SUCCESS ();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetFlowBlock (token_t **input_token, program_t *prog)
{
    PREPARE();

    if ((node = GetIfBlock (&token, prog)) == nullptr) {
        if ((node = GetWhileBlock (&token, prog)) == nullptr) {
            if (isKEYWORD (OPEN_BLOCK)) {
                token++;
                TRY (node = GetBody (&token, prog));
                CHECK_KEYWORD(CLOSE_BLOCK);
            } else {
                TRY (node = GetBody (&token, prog));
            }
        }
    }

    SUCCESS ();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() tree::del_node (cond);

static tree::node_t *GetWhileBlock (token_t **input_token, program_t *prog)
{
    PREPARE();
    tree::node_t *cond = nullptr;

    CHECK_KEYWORD (L_BRACKET);
    TRY (cond = GetExpression (&token, prog));
    CHECK_KEYWORD (R_BRACKET);
    CHECK_KEYWORD (WHILE);

    CHECK_KEYWORD (OPEN_BLOCK);
    TRY (node = GetBody (&token, prog));
    CHECK_KEYWORD (CLOSE_BLOCK);

    node = tree::new_node (node_type_t::WHILE, 0, cond, node);
    SUCCESS();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() tree::del_node (cond);

static tree::node_t *GetIfBlock (token_t **input_token, program_t *prog)
{
    PREPARE ();
    tree::node_t *cond     = nullptr;
    tree::node_t *tmp_node = nullptr;

    CHECK_KEYWORD (L_BRACKET);
    TRY (cond = GetExpression (&token, prog));
    CHECK_KEYWORD (R_BRACKET);
    CHECK_KEYWORD (IF);

    CHECK_KEYWORD (OPEN_BLOCK);
    TRY (node = GetSubProgram (&token, prog));
    CHECK_KEYWORD (CLOSE_BLOCK);

    node = tree::new_node (node_type_t::ELSE, 0, nullptr, node);

    if (isKEYWORD (ELSE))
    {
        token++;

        CHECK_KEYWORD (OPEN_BLOCK);
        TRY (tmp_node = GetSubProgram (&token, prog));
        CHECK_KEYWORD (CLOSE_BLOCK);

        node->left  = node->right;
        node->right = tmp_node;
    }

    node = tree::new_node (node_type_t::IF, 0, cond, node);
    SUCCESS ();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetBody (token_t **input_token, program_t *prog)
{
    PREPARE ();
    tree::node_t *next_line = nullptr;

    TRY (node = GetLine (&token, prog));

    while ((next_line = GetLine (&token, prog)) != nullptr)
    {
        node = tree::new_node (node_type_t::FICTIOUS, 0, node, next_line);
    }

    SUCCESS();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() tree::del_node (var_def);

static tree::node_t *GetLine (token_t **input_token, program_t *prog)
{
    PREPARE ();

    tree::node_t *var_def = nullptr;

    CHECK_KEYWORD (BREAK);
    TRY (node = GetExpression (&token, prog));

    if (isKEYWORD (ASSIG))
    {
        token++;

        EXPECT (isTYPE (NAME));
        int var_name = token->name;
        SET_NAME_TYPE (token->name, var_name, var);
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

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetExpression (token_t **input_token, program_t *prog)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;

    TRY (node = GetOrOperand (&token, prog));

    while (isKEYWORD (OR))
    {
        token++;

        TRY (node_rhs = GetOrOperand (&token, prog));

        node = tree::new_node (node_type_t::OP, op_t::OR, node_rhs, node);
    }

    SUCCESS();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetOrOperand (token_t **input_token, program_t *prog)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;

    TRY (node = GetAndOperand (&token, prog));

    while (isKEYWORD (AND))
    {
        token++;

        TRY (node_rhs = GetAndOperand (&token, prog));

        node = tree::new_node (node_type_t::OP, op_t::AND, node_rhs, node);
    }

    SUCCESS ();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}
#define TRANSLATE_KEYWORD(op_in)  \
    case token::keyword::op_in:   \
        node = tree::new_node (tree::node_type_t::OP, tree::op_t::op_in, node_rhs, node); \
        break;

static tree::node_t *GetAndOperand (token_t **input_token, program_t *prog)
{
    PREPARE ();

    TRY (node = GetCompOperand (&token, prog));

    if (isKEYWORD (GE) || isKEYWORD (LE) || isKEYWORD (GT) ||
        isKEYWORD (LT) || isKEYWORD (EQ) || isKEYWORD (NEQ))
    {
        tree::node_t *node_rhs = nullptr;
        token::keyword comp_op = token->keyword;
        token++;
        TRY (node_rhs = GetCompOperand (&token, prog));
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (comp_op) {
            TRANSLATE_KEYWORD (GE);
            TRANSLATE_KEYWORD (LE);
            TRANSLATE_KEYWORD (LT);
            TRANSLATE_KEYWORD (GT);
            TRANSLATE_KEYWORD (EQ);
            TRANSLATE_KEYWORD (NEQ);
            
            default: assert (0 && "Logic error: incorrect if condition");
        }
        #pragma GCC diagnostic pop
    }

    SUCCESS ();
}

#undef TRANSLATE_KEYWORD
#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetCompOperand (token_t **input_token, program_t *prog)
{
    PREPARE();
    op_t op = tree::op_t::INPUT; // Poison value
    tree::node_t *node_rhs = nullptr;

    TRY (node = GetAddOperand (&token, prog));

    while (isOP(ADD) || isOP(SUB))
    {
        op = isOP (ADD) ? op_t::ADD : op_t::SUB;
        token++;

        TRY (node_rhs = GetAddOperand (&token, prog));

        node = tree::new_node (node_type_t::OP, op, node_rhs, node);
    }

    SUCCESS();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetAddOperand (token_t **input_token, program_t *prog)
{
    PREPARE ();

    tree::node_t *node_rhs = nullptr;
    op_t op = tree::op_t::INPUT; // Poison value

    TRY (node = GetMulOperand (&token, prog));

    while (isOP(MUL) || isOP(DIV))
    {
        op = isOP (MUL) ? op_t::MUL : op_t::DIV;
        token++;

        TRY (node_rhs = GetMulOperand (&token, prog));

        node = tree::new_node (node_type_t::OP, op, node_rhs, node);
    }

    SUCCESS();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetMulOperand (token_t **input_token, program_t *prog)
{
    PREPARE ();

    TRY (node = GetGeneralOperand (&token, prog));

    if (isKEYWORD (NOT))
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::NOT, nullptr, node);
    }

    SUCCESS ();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetGeneralOperand (token_t **input_token, program_t *prog)
{
    PREPARE ();

    if ((node = GetQuant(&token, prog)) != nullptr)
    {
        SUCCESS ();
    }

    CHECK_KEYWORD(L_BRACKET);
    TRY (node = GetExpression (&token, prog));
    CHECK_KEYWORD (R_BRACKET);

    SUCCESS ();
}

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetQuant (token_t **input_token, program_t *prog)
{
    PREPARE();
    int real_name = -1;

    if (isTYPE(NAME))
    {
        SET_NAME_TYPE (token->name, real_name, var);
        node = tree::new_node (tree::node_type_t::VAR, real_name);
        token++;
    }
    else if (isTYPE(VAL))
    {
        node = tree::new_node (tree::node_type_t::VAL, token->val);
        token++;
    }
    else if (isKEYWORD (L_BRACKET))
    {
        if ((node = GetBuiltInFunc (&token, prog)) != nullptr)
        {
            SUCCESS ();
        }

        token++;
        tree::node_t *node_param = nullptr;

        if (!isKEYWORD (R_BRACKET))
        {
            TRY (node = GetExpression (&token, prog));

            while (isKEYWORD (SEP))
            {
                token++;

                TRY (node_param = GetExpression (&token, prog));
                node = tree::new_node (node_type_t::FICTIOUS, 0, node_param, node);
            }
        }

        CHECK_KEYWORD (R_BRACKET);
        EXPECT (isTYPE (NAME));
        SET_NAME_TYPE (token->name, real_name, func);
        node = tree::new_node (node_type_t::FUNC_CALL, real_name, nullptr, node);
        
        token++;
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

#undef EXTRA_CLEAR_ON_ERROR

// -------------------------------------------------------------------------------------------------

#define EXTRA_CLEAR_ON_ERROR() {;}

static tree::node_t *GetBuiltInFunc (token_t **input_token, program_t *prog)
{
    PREPARE();

    CHECK_KEYWORD(L_BRACKET);
    TRY (node = GetExpression (&token, prog));
    CHECK_KEYWORD(R_BRACKET);

    EXPECT (token->type == token::type_t::KEYWORD);

    if (token->keyword == token::keyword::PRINT)
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::OUTPUT, nullptr, node);
    }
    else if (token->keyword == token::keyword::SQRT)
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::SQRT, nullptr, node);
    }
    else if (token->keyword == token::keyword::SIN)
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::SIN, nullptr, node);
    }
    else if (token->keyword == token::keyword::COS)
    {
        token++;
        node = tree::new_node (node_type_t::OP, op_t::COS, nullptr, node);
    } else {
        EXPECT (0);
    }

    SUCCESS();    
}

#undef EXTRA_CLEAR_ON_ERROR
