#include <cassert>
#include "codegen.h"
#include "lexer.h"

static void subtree_codegen (tree::node_t *node, program_t *program, FILE *stream);

static void codegen_if        (tree::node_t *node, program_t *prog, FILE *stream);
static void codegen_while     (tree::node_t *node, program_t *prog, FILE *stream);
static void codegen_op        (tree::node_t *node, program_t *prog, FILE *stream);
static void codegen_func_def  (tree::node_t *node, program_t *prog, FILE *stream);
static void codegen_func_call (tree::node_t *node, program_t *prog, FILE *stream);

static bool codegen_func_args (tree::node_t *node, program_t *prog, FILE *stream,
                                                                                bool not_first_arg);

// -------------------------------------------------------------------------------------------------

#define EMIT(fmt, ...) fprintf (stream, fmt, ##__VA_ARGS__)

// -------------------------------------------------------------------------------------------------

void program::codegen (program_t *program, FILE *stream)
{
    assert (program != nullptr && "invalid pointer");
    assert (stream  != nullptr && "invalid pointer");

    EMIT ("~sya~\n");
    subtree_codegen (program->ast, program, stream);
    EMIT ("\n~nya~\n");
}

// -------------------------------------------------------------------------------------------------

#define NAME(node_type) prog->node_type##_names.names[node->data]

#define NOT_TYPE(node, bad_type) node->type != tree::node_type_t::bad_type

#define WRAP_BREAK(node)                                                                \
{                                                                                       \
    if (node != nullptr)                                                                \
    {                                                                                   \
        if (NOT_TYPE (node, FICTIOUS) && NOT_TYPE (node, IF) && NOT_TYPE(node, WHILE)&& \
            NOT_TYPE(node, FUNC_DEF))                                                   \
        {                                                                               \
            EMIT (";");                                                                 \
        }                                                                               \
        subtree_codegen (node, prog, stream);                                           \
    }                                                                                   \
}

static void subtree_codegen (tree::node_t *node, program_t *prog, FILE *stream)
{
    assert (prog    != nullptr && "invalid pointer");
    assert (stream  != nullptr && "invalid pointer");
    assert (node    != nullptr && "invalid pointer");
 
    switch (node->type)
    {
        case tree::node_type_t::FICTIOUS:
            WRAP_BREAK (node->left);
            EMIT ("\n");
            WRAP_BREAK (node->right);
            break;

        case tree::node_type_t::VAL:     EMIT ("%d",           node->data); break;
        case tree::node_type_t::VAR:     EMIT ("%s",           NAME (var)); break;
        case tree::node_type_t::VAR_DEF: EMIT ("0 = %s let", NAME (var)); break;
        
        case tree::node_type_t::IF:         codegen_if        (node, prog, stream); break;
        case tree::node_type_t::WHILE:      codegen_while     (node, prog, stream); break;
        case tree::node_type_t::OP:         codegen_op        (node, prog, stream); break;     
        case tree::node_type_t::FUNC_DEF:   codegen_func_def  (node, prog, stream); break;     
        case tree::node_type_t::FUNC_CALL:  codegen_func_call (node, prog, stream); break;     
        
        case tree::node_type_t::RETURN:
            subtree_codegen (node->right, prog, stream);
            EMIT (" return");
            break;

        case tree::node_type_t::ELSE:     assert (0 && "Compiled in if node");
        case tree::node_type_t::NOT_SET:  assert (0 && "Invalid node");
        default:                          assert (0 && "Unexpected node");
        
    }
}


// -------------------------------------------------------------------------------------------------

static void codegen_if(tree::node_t *node, program_t *prog, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::IF && "invalid call");

    EMIT ("(");
    subtree_codegen (node->left, prog, stream);
    EMIT (") if\n");
    EMIT ("{\n");

    if (node->right->type == tree::node_type_t::ELSE) {
        subtree_codegen (node->right->left,  prog, stream);
        EMIT ("\n} else {\n");
        subtree_codegen (node->right->right, prog, stream);
    } else {
        subtree_codegen (node->right, prog, stream);
    }

    EMIT ("\n}\n");
}

// -------------------------------------------------------------------------------------------------

static void codegen_while(tree::node_t *node, program_t *prog, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::WHILE && "invalid call");

    EMIT ("(");
    subtree_codegen (node->left, prog, stream);
    EMIT (") while\n");
    EMIT ("{\n");

    subtree_codegen (node->right, prog, stream);
    EMIT ("\n}\n");
}

// -------------------------------------------------------------------------------------------------

#define CODEGEN_KEYWORD(kw)                      \
    EMIT (kw " ");                               \
    subtree_codegen (node->right, prog, stream); \
    break;                                       \

#define CODEGEN_BIN_OP(op)                   \
EMIT ("(");                                  \
subtree_codegen (node->right,  prog, stream);\
EMIT (") " op " (");                         \
subtree_codegen (node->left, prog, stream);  \
EMIT (")");                                  \
break;

static void codegen_op(tree::node_t *node, program_t *prog, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::OP && "invalid call");

    switch ((tree::op_t) node->data)
    {
        case tree::op_t::ADD:   CODEGEN_BIN_OP("+");
        case tree::op_t::SUB:   CODEGEN_BIN_OP("-");
        case tree::op_t::MUL:   CODEGEN_BIN_OP("*");
        case tree::op_t::DIV:   CODEGEN_BIN_OP("/");
        case tree::op_t::EQ:    CODEGEN_BIN_OP("==");
        case tree::op_t::GT:    CODEGEN_BIN_OP(">");
        case tree::op_t::LT:    CODEGEN_BIN_OP("<");
        case tree::op_t::GE:    CODEGEN_BIN_OP(">=");
        case tree::op_t::LE:    CODEGEN_BIN_OP("<=");
        case tree::op_t::NEQ:   CODEGEN_BIN_OP("!=");
        case tree::op_t::AND:   CODEGEN_BIN_OP("&&");
        case tree::op_t::OR:    CODEGEN_BIN_OP("||");
        
        case tree::op_t::ASSIG: 
            EMIT ("(");                                  
            subtree_codegen (node->right,  prog, stream);
            EMIT (") = ");                               
            subtree_codegen (node->left, prog, stream);  
            break;

        case tree::op_t::SQRT:      CODEGEN_KEYWORD ("__builtin_sqrt__");
        case tree::op_t::INPUT:     CODEGEN_KEYWORD ("__builtint_input__");
        case tree::op_t::OUTPUT:    CODEGEN_KEYWORD ("__builtin_print__");
        
        case tree::op_t::NOT:
            EMIT ("(");
            subtree_codegen (node->right, prog, stream);
            EMIT (")! ");
            break;

        default:
            assert (0 && "Unexpected node");
    }
}

#undef CODEGEN_BIN_OP
#undef CODEGEN_KEYWORD

// -------------------------------------------------------------------------------------------------

static void codegen_func_def(tree::node_t *node, program_t *prog, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::FUNC_DEF && "invalid call");

    EMIT ("(");
    codegen_func_args (node->left, prog, stream, false);
    EMIT (") %s fn\n", NAME(func));
    EMIT ("[\n");
    subtree_codegen (node->right, prog, stream);
    EMIT ("\n]\n");
}

// -------------------------------------------------------------------------------------------------

static bool codegen_func_args (tree::node_t *node, program_t *prog, FILE *stream, bool not_first_arg)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    if (node->type != tree::node_type_t::FICTIOUS)
    {
        subtree_codegen (node, prog, stream);
        if (not_first_arg) {
            EMIT (",");
        } 
        return true;
    }

    if (node->left != nullptr) {
        not_first_arg = codegen_func_args (node->left, prog, stream, not_first_arg)
                                                                                || not_first_arg;
    }

    if (node->right != nullptr) {
        not_first_arg = codegen_func_args (node->right, prog, stream, not_first_arg)
                                                                                || not_first_arg;
    }

    return not_first_arg;
}

// -------------------------------------------------------------------------------------------------

static void codegen_func_call (tree::node_t *node, program_t *prog, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::FUNC_CALL && "invalid call");

    EMIT ("(");
    codegen_func_args (node->right, prog, stream, false);
    EMIT (") %s", NAME (func));
}