#include <cassert>
#include "optimizer.h"


static bool subtree_optimize_const (tree::node_t *node);
static bool optimize_const_calc (tree::node_t *node);
static bool optimize_const_comp (tree::node_t *node);

// -------------------------------------------------------------------------------------------------

#define isVAL(_node) (_node->type == tree::node_type_t::VAL)
#define isVALorNIL(_node) (_node == nullptr || _node->type == tree::node_type_t::VAL)

#define OPTIMIZE(_node)                     \
{                                           \
    if (subtree_optimize_const (_node))     \
    {                                       \
        changed = true;                     \
    }                                       \
}

// -------------------------------------------------------------------------------------------------

void optimize (tree::node_t *node)
{
    while (subtree_optimize_const (node))
    {
        ;
    }
}

// -------------------------------------------------------------------------------------------------

static bool subtree_optimize_const (tree::node_t *node)
{
    assert (node != nullptr);

    bool changed = false;

    if (node->type != tree::node_type_t::OP) {
        if (node->left)  { OPTIMIZE (node->left ); }
        if (node->right) { OPTIMIZE (node->right); }

        return changed;
    }

    switch ((tree::op_t) node->data) {
        case tree::op_t::ADD:
        case tree::op_t::SUB:
        case tree::op_t::MUL:
            return optimize_const_calc (node);

        case tree::op_t::EQ:
        case tree::op_t::GT:
        case tree::op_t::LT:
        case tree::op_t::GE:
        case tree::op_t::LE:
        case tree::op_t::NEQ:
        case tree::op_t::NOT:
        case tree::op_t::AND:
        case tree::op_t::OR:
            return optimize_const_comp (node);

        case tree::op_t::INPUT:
        case tree::op_t::OUTPUT:
        case tree::op_t::ASSIG:
        case tree::op_t::SQRT:
        case tree::op_t::DIV:
            return subtree_optimize_const (node->right); 
    }
}

// -------------------------------------------------------------------------------------------------
// STATIC SECTION
// -------------------------------------------------------------------------------------------------

#define SET_VALUE(val)                                      \
    tree::change_node (node, tree::node_type_t::VAL, val);  \
    del_childs (node);                                      \

// -------------------------------------------------------------------------------------------------

static bool optimize_const_calc (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");
    assert (node->left  && "add/mul op node must have right child");
    assert (node->right && "add/mul op node must have right child");

    bool changed = false;
    OPTIMIZE (node->left);
    OPTIMIZE (node->right);

    if (isVAL (node->left) && isVAL (node->right))
    {
        switch ((tree::op_t) node->data) {
            case tree::op_t::ADD: SET_VALUE (node->left->data + node->right->data); break;
            case tree::op_t::SUB: SET_VALUE (node->left->data - node->right->data); break;
            case tree::op_t::MUL: SET_VALUE (node->left->data * node->right->data); break;

            default:
                assert (0 && "Unexpected node"); 
        }
    }

    return changed;
}

// -------------------------------------------------------------------------------------------------

#define SET_COMP_RESULT(comp)                       \
{                                                   \
    if (node->left->data comp node->right->data) {  \
        SET_VALUE (1);                              \
    } else {                                        \
        SET_VALUE (0);                              \
    }                                               \
    break;                                          \
}

static bool optimize_const_comp (tree::node_t *node)
{
    assert (node        != nullptr && "invalid pointer");
    assert (node->right != nullptr && "invalid node");

    bool changed = false;
    if (node->left) {
        OPTIMIZE (node->left);
    }
    OPTIMIZE (node->right);

    if (isVALorNIL (node->left) && isVALorNIL (node->right))
    {
        switch ((tree::op_t) node->data) {
            case tree::op_t::EQ:  SET_COMP_RESULT (==);
            case tree::op_t::GT:  SET_COMP_RESULT (>);
            case tree::op_t::LT:  SET_COMP_RESULT (<);
            case tree::op_t::GE:  SET_COMP_RESULT (>=);
            case tree::op_t::LE:  SET_COMP_RESULT (<=);
            case tree::op_t::NEQ: SET_COMP_RESULT (!=);
            case tree::op_t::AND: SET_COMP_RESULT (&&);
            case tree::op_t::OR:  SET_COMP_RESULT (||);
            
            case tree::op_t::NOT: SET_VALUE(!node->right->data); break;

            default: assert (0 && "Invalid call");
        }
    }

    return changed;
}