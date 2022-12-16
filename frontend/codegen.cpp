#include <cassert>
#include "codegen.h"
#include "lexer.h"

static void subtree_codegen (tree::node_t *node, program_t *program, int step_cnt, FILE *stream, bool no_breakes = false);

static void codegen_if        (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream);
static void codegen_while     (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream);
static void codegen_op        (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream);
static void codegen_func_def  (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream);
static void codegen_func_call (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream);

static bool codegen_func_args (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream,
                                                                                bool not_first_arg);

// -------------------------------------------------------------------------------------------------

#define EMIT(fmt, ...) fprintf (stream, fmt, ##__VA_ARGS__)

#define NEW_LINE()                          \
{                                           \
    EMIT ("\n");                            \
    for (int i = 0; i < step_cnt; ++i)      \
    {                                       \
        EMIT ("    ");                      \
    }                                       \
}

// -------------------------------------------------------------------------------------------------

void program::codegen (program_t *program, FILE *stream)
{
    assert (program != nullptr && "invalid pointer");
    assert (stream  != nullptr && "invalid pointer");

    EMIT ("~sya~\n");
    subtree_codegen (program->ast, program, 0, stream);
    EMIT ("\n~nya~\n");
}

// -------------------------------------------------------------------------------------------------

#define NAME(node_type) prog->node_type##_names.names[node->data]

#define NOT_TYPE(node, bad_type) node->type != tree::node_type_t::bad_type

#define WRAP_BREAK(node)                                                                \
{                                                                                       \
    if (node != nullptr)                                                                \
    {                                                                                   \
        if (!no_breaks && NOT_TYPE (node, FICTIOUS) && NOT_TYPE (node, IF) && NOT_TYPE(node, WHILE)&& \
            NOT_TYPE(node, FUNC_DEF))                                                   \
        {                                                                               \
            EMIT (";");                                                                 \
        }                                                                               \
        subtree_codegen (node, prog, step_cnt, stream);                                           \
    }                                                                                   \
}

static void subtree_codegen (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream, bool no_breaks)
{
    assert (prog    != nullptr && "invalid pointer");
    assert (stream  != nullptr && "invalid pointer");
    assert (node    != nullptr && "invalid pointer");
 
    switch (node->type)
    {
        case tree::node_type_t::FICTIOUS:
            if (node->left)
            {
                WRAP_BREAK (node->left);
                NEW_LINE();
            }
            WRAP_BREAK (node->right);
            break;

        case tree::node_type_t::VAL:     EMIT ("%d",         node->data); break;
        case tree::node_type_t::VAR:     EMIT ("%s",         NAME (var)); break;
        case tree::node_type_t::VAR_DEF: EMIT ("0 = %s let", NAME (var)); break;
        
        case tree::node_type_t::IF:         codegen_if        (node, prog, step_cnt, stream); break;
        case tree::node_type_t::WHILE:      codegen_while     (node, prog, step_cnt, stream); break;
        case tree::node_type_t::OP:         codegen_op        (node, prog, step_cnt, stream); break;     
        case tree::node_type_t::FUNC_DEF:   codegen_func_def  (node, prog, step_cnt, stream); break;     
        case tree::node_type_t::FUNC_CALL:  codegen_func_call (node, prog, step_cnt, stream); break;     
        
        case tree::node_type_t::RETURN:
            subtree_codegen (node->right, prog, step_cnt, stream, true);
            EMIT (" return");
            break;

        case tree::node_type_t::ELSE:     assert (0 && "Compiled in if node");
        case tree::node_type_t::NOT_SET:  assert (0 && "Invalid node");
        default:                          assert (0 && "Unexpected node");
    }
}


// -------------------------------------------------------------------------------------------------

static void codegen_if(tree::node_t *node, program_t *prog, int step_cnt, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::IF && "invalid call");

    EMIT ("(");
    subtree_codegen (node->left, prog, step_cnt, stream, true);
    EMIT (") if");
    NEW_LINE();
    EMIT ("{");
    NEW_LINE();

    if (node->right->left != nullptr) {
        subtree_codegen (node->right->left,  prog, step_cnt + 1, stream);
        NEW_LINE();
        EMIT ("} else {");
        NEW_LINE();
        subtree_codegen (node->right->right, prog, step_cnt + 1, stream);
    } else {
        subtree_codegen (node->right->right, prog, step_cnt + 1, stream);
    }

    NEW_LINE();
    EMIT ("}");
    NEW_LINE();
}

// -------------------------------------------------------------------------------------------------

static void codegen_while(tree::node_t *node, program_t *prog, int step_cnt, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::WHILE && "invalid call");

    EMIT ("(");
    subtree_codegen (node->left, prog, step_cnt, stream, true);
    EMIT (") while");
    NEW_LINE();
    EMIT ("{");
    step_cnt++;
    NEW_LINE();
    subtree_codegen (node->right, prog, step_cnt, stream);
    step_cnt--;
    NEW_LINE();
    EMIT ("}");
    NEW_LINE();
}

// -------------------------------------------------------------------------------------------------

#define CODEGEN_KEYWORD(kw)                                    \
    EMIT (kw " ");                                             \
    if (node->right) {                                         \
        subtree_codegen (node->right, prog, step_cnt, stream, true); \
    }                                                          \
    break;                                                     \

#define CODEGEN_BIN_OP(op)                              \
EMIT ("(");                                             \
subtree_codegen (node->right, prog, step_cnt, stream, true);  \
EMIT (") " op " (");                                    \
subtree_codegen (node->left,  prog, step_cnt, stream, true);  \
EMIT (")");                                             \
break;

static void codegen_op(tree::node_t *node, program_t *prog, int step_cnt, FILE *stream)
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
            subtree_codegen (node->right, prog, step_cnt, stream);
            EMIT (") = ");                               
            subtree_codegen (node->left, prog, step_cnt, stream);  
            break;

        case tree::op_t::SQRT:      CODEGEN_KEYWORD ("__builtin_sqrt__");
        case tree::op_t::INPUT:     CODEGEN_KEYWORD ("__builtint_input__");
        case tree::op_t::OUTPUT:    CODEGEN_KEYWORD ("__builtin_print__");
        
        case tree::op_t::NOT:
            EMIT ("(");
            subtree_codegen (node->right, prog, step_cnt, stream);
            EMIT (")! ");
            break;

        default:
            assert (0 && "Unexpected node");
    }
}

#undef CODEGEN_BIN_OP
#undef CODEGEN_KEYWORD

// -------------------------------------------------------------------------------------------------

static void codegen_func_def(tree::node_t *node, program_t *prog, int step_cnt, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::FUNC_DEF && "invalid call");

    EMIT ("(");
    codegen_func_args (node->left, prog, step_cnt, stream, false);
    EMIT (") %s fn", NAME(func));
    NEW_LINE();
    EMIT ("[");
    step_cnt++;
    NEW_LINE();
    subtree_codegen (node->right, prog, step_cnt, stream);
    step_cnt--;
    NEW_LINE();
    EMIT ("]");
    NEW_LINE();
}

// -------------------------------------------------------------------------------------------------

static bool codegen_func_args (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream, bool not_first_arg)
{
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    if (node == nullptr)
    {
        return false;
    }

    if (node->type != tree::node_type_t::FICTIOUS)
    {
        if (not_first_arg) {
            EMIT (",");
        } 
        subtree_codegen (node, prog, step_cnt, stream, true);
        return true;
    }

    if (node->left != nullptr) {
        not_first_arg = codegen_func_args (node->left, prog, step_cnt, stream, not_first_arg)
                                                                                || not_first_arg;
    }

    if (node->right != nullptr) {
        not_first_arg = codegen_func_args (node->right, prog, step_cnt, stream, not_first_arg)
                                                                                || not_first_arg;
    }

    return not_first_arg;
}

// -------------------------------------------------------------------------------------------------

static void codegen_func_call (tree::node_t *node, program_t *prog, int step_cnt, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (prog   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::FUNC_CALL && "invalid call");

    EMIT ("(");
    codegen_func_args (node->right, prog, step_cnt, stream, false);
    EMIT (") %s", NAME (func));
}