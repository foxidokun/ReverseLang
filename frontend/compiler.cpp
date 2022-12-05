#include <assert.h>
#include <cstdlib>
#include "compiler.h"

#define EMIT(fmt, ...)                                                              \
{                                                                                   \
    sprintf (buf, fmt, ##__VA_ARGS__);                                              \
    fprintf (stream, "%-30s; %-20s (%s:%d)\n", buf,  __func__, __FILE__, __LINE__);     \
}

// -------------------------------------------------------------------------------------------------

const int DEFAULT_VARS_CAPACITY = 16;
const int BUF_SIZE = 35;

// -------------------------------------------------------------------------------------------------

static void subtree_compile (compiler_t *compiler, tree::node_t *node, FILE *stream);
static void compile_op      (compiler_t *compiler, tree::node_t *node, FILE *stream);

void register_var (compiler_t *compiler, int number);
int get_offset    (compiler_t *compiler, int number);

// -------------------------------------------------------------------------------------------------

void compiler::ctor (compiler_t *compiler)
{
    assert (compiler != nullptr && "invalid pointer");

    compiler->vars.name_indexes = (int *) calloc (DEFAULT_VARS_CAPACITY, sizeof (int));
    compiler->vars.capacity = DEFAULT_VARS_CAPACITY;
    compiler->vars.size     = 0;
}

void compiler::dtor (compiler_t *compiler)
{
    assert (compiler != nullptr && "invalid pointer");

    free (compiler->vars.name_indexes);
}

// -------------------------------------------------------------------------------------------------

void compiler::compile (tree::node_t *node, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    compiler_t compiler = {};
    ctor (&compiler);

    char buf[BUF_SIZE] = "";

    EMIT ("push 0");
    EMIT ("pop rdx ; Init rdx");
    EMIT ("\n\n; Here we go again");

    subtree_compile (&compiler, node, stream);

    EMIT ("halt");

    dtor (&compiler);
}

// -------------------------------------------------------------------------------------------------

static void subtree_compile (compiler_t *compiler, tree::node_t *node, FILE *stream)
{
    assert (stream != nullptr && "invalid pointer");

    if (node == nullptr) {
        return;
    }

    char buf[BUF_SIZE] = "";

    switch (node->type)
    {
        case tree::node_type_t::FICTIOUS:
            subtree_compile (compiler, node->left,  stream);
            subtree_compile (compiler, node->right, stream);
            break;

        case tree::node_type_t::VAL:
            EMIT ("push %d ; val node", node->data);
            break;

        case tree::node_type_t::VAR:
            EMIT ("push [rdx+%d]", get_offset (compiler, node->data));
            break;

        case tree::node_type_t::VAR_DEF:
            register_var (compiler, node->data);
            break;
        
        case tree::node_type_t::OP:
            compile_op (compiler, node, stream);
            break;

        case tree::node_type_t::IF:
        case tree::node_type_t::ELSE:
        case tree::node_type_t::WHILE:
        case tree::node_type_t::FUNC_DEF:
        case tree::node_type_t::FUNC_CALL:
        case tree::node_type_t::RETURN:
            assert (0 && "Not Implemented");

        case tree::node_type_t::NOT_SET:
            assert (0 && "Invalid node");
        
        default:
            assert (0 && "Unexpected node");
    }
}

// -------------------------------------------------------------------------------------------------

#define EMIT_BINARY_OP(opcode)                       \
    subtree_compile (compiler, node->left,  stream); \
    subtree_compile (compiler, node->right, stream); \
    EMIT (opcode);                                   \
    break;

static void compile_op (compiler_t *compiler, tree::node_t *node, FILE *stream)
{
    assert (compiler != nullptr && "invalid pointer");
    assert (node     != nullptr && "invalid pointer");
    assert (stream   != nullptr && "invalid pointer");

    assert (node->type == tree::node_type_t::OP);

    char buf[BUF_SIZE] = "";

    switch ((tree::op_t) node->data)
    {
        case tree::op_t::ADD: EMIT_BINARY_OP ("add");
        case tree::op_t::SUB: EMIT_BINARY_OP ("sub");
        case tree::op_t::MUL: EMIT_BINARY_OP ("mul");
        case tree::op_t::DIV: EMIT_BINARY_OP ("div");

        case tree::op_t::OUTPUT:
            subtree_compile (compiler, node->right, stream);
            EMIT ("out");
            break;

        case tree::op_t::ASSIG:
            subtree_compile (compiler, node->right, stream);
            EMIT ("pop [rdx+%d] ; Assig", get_offset (compiler, node->left->data));
            break;

        case tree::op_t::POW:
        case tree::op_t::INPUT:
        case tree::op_t::EQ:
        case tree::op_t::GT:
        case tree::op_t::LT:
        case tree::op_t::GE:
        case tree::op_t::LE:
        case tree::op_t::NEQ:
        case tree::op_t::NOT:
        case tree::op_t::AND:
        case tree::op_t::OR:
            assert (0 && "Not implemented");

        default:
            assert (0 && "Unexpected op type");
    }
}

#undef EMIT_BINARY_OP

// -------------------------------------------------------------------------------------------------

void register_var (compiler_t *compiler, int number)
{
    assert (compiler != nullptr && "invalid pointer");

    if (compiler->vars.size == compiler->vars.capacity)
    {
        compiler->vars.name_indexes = (int *) realloc (compiler->vars.name_indexes, 2 * compiler->vars.capacity * sizeof (int));
        compiler->vars.capacity    *= 2;
    }

    compiler->vars.name_indexes[compiler->vars.size] = number;
    compiler->vars.size++;
}

int get_offset (compiler_t *compiler, int number)
{
    assert (compiler != nullptr && "Invalid pointer");

    for (unsigned int i = 0; i < compiler->vars.size; ++i)
    {
        if (compiler->vars.name_indexes[i] == number)
        {
            return (int) i;
        }
    }

    assert (0 && "broken tree: var not registered");
}