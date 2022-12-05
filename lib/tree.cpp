#include <assert.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "common.h"
#include "file.h"
#include "log.h"

#include "tree.h"


// -------------------------------------------------------------------------------------------------
// CONST SECTION
// -------------------------------------------------------------------------------------------------

const int REASON_LEN   = 50;
const int MAX_NODE_LEN = 12;

const char PREFIX[] = "digraph {\nnode [shape=record,style=\"filled\"]\nsplines=spline;\n";
static const size_t DUMP_FILE_PATH_LEN = 20;
static const char DUMP_FILE_PATH_FORMAT[] = "dump/%d.grv";

// -------------------------------------------------------------------------------------------------
// STATIC PROTOTYPES SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param);

static bool node_codegen (tree::node_t *node, void *stream_void, bool cont);

static const char *get_op_name (tree::op_t op);
static void format_node (char *buf, const tree::node_t *node);


// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

void tree::ctor (tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    tree->head_node = nullptr;
}

void tree::dtor (tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    del_node (tree->head_node);
}

// -------------------------------------------------------------------------------------------------

bool tree::dfs_exec (tree_t *tree, walk_f pre_exec,  void *pre_param,
                                   walk_f in_exec,   void *in_param,
                                   walk_f post_exec, void *post_param)
{
    assert (tree != nullptr && "invalid pointer");
    assert (tree->head_node != nullptr && "invalid tree");

    return dfs_recursion (tree->head_node, pre_exec,  pre_param,
                                           in_exec,   in_param,
                                           post_exec, post_param);
}

bool tree::dfs_exec (node_t *node, walk_f pre_exec,  void *pre_param,
                                   walk_f in_exec,   void *in_param,
                                   walk_f post_exec, void *post_param)
{
    assert (node != nullptr && "invalid pointer");

    return dfs_recursion (node, pre_exec,  pre_param,
                                in_exec,   in_param,
                                post_exec, post_param);
}

// -------------------------------------------------------------------------------------------------

void tree::change_node (node_t *node, node_type_t type, int data)
{
    assert (node != nullptr && "invalid pointer");

    node->data = data;
    node->type = type;
}

// -------------------------------------------------------------------------------------------------

void tree::move_node (node_t *dest, node_t *src)
{
    assert (dest != nullptr && "invalid pointer");
    assert (src  != nullptr && "invalid pointer");

    memcpy (dest, src, sizeof (node_t));

    free (src);
}

// -------------------------------------------------------------------------------------------------

#define NEW_NODE_IN_CASE(type, field)               \
    case tree::node_type_t::type:                   \
        node_copy = tree::new_node (node->field);   \
        if (node_copy == nullptr) return nullptr;   \
        break;

tree::node_t *tree::copy_subtree (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    tree::node_t *node_copy = new_node (node->type, node->data);

    if (node->right != nullptr) {
        node_copy->right = copy_subtree (node->right);
    } else {
        node_copy->right = nullptr;
    }

    if (node->left != nullptr) {
        node_copy->left = copy_subtree (node->left);
    } else {
        node_copy->left = nullptr;
    }

    return node_copy;
}

#undef NEW_NODE_IN_CASE

// -------------------------------------------------------------------------------------------------

int tree::graph_dump (tree_t *tree, const char *reason_fmt, ...)
{
    assert (tree       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, reason_fmt);

    int res = graph_dump (tree->head_node, reason_fmt, args);

    va_end (args);
    return res;
}


int tree::graph_dump (node_t *node, const char *reason_fmt, ...)
{
    assert (node       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, reason_fmt);

    int res = graph_dump (node, reason_fmt, args);

    va_end (args);
    return res;
}

int tree::graph_dump (node_t *node, const char *reason_fmt, va_list args)
{
    assert (node       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    static int counter = 0;
    counter++;

    char filepath[DUMP_FILE_PATH_LEN+1] = "";    
    sprintf (filepath, DUMP_FILE_PATH_FORMAT, counter);

    FILE *dump_file = fopen (filepath, "w");
    if (dump_file == nullptr)
    {
        LOG (log::ERR, "Failed to open dump file '%s'", filepath);
        return counter;
    }

    fprintf (dump_file, PREFIX);

    dfs_exec (node, node_codegen, dump_file,
                    nullptr, nullptr,
                    nullptr, nullptr);

    fprintf (dump_file, "}\n");

    fclose (dump_file);

    char cmd[2*DUMP_FILE_PATH_LEN+20+1] = "";
    sprintf (cmd, "dot -T png -o %s.png %s", filepath, filepath);
    if (system (cmd) != 0)
    {
        LOG (log::ERR, "Failed to execute '%s'", cmd);
    }

    #if HTML_LOGS
        FILE *stream = get_log_stream ();

        fprintf  (stream, "<h2>List dump: ");
        vfprintf (stream, reason_fmt, args);
        fprintf  (stream, "</h2>");

        fprintf (stream, "\n\n<img src=\"%s.png\">\n\n", filepath);
    #else
        char buf[REASON_LEN] = "";
        vsprintf (buf, reason_fmt, args);
        LOG (log::INF, "Dump path: %s.png, reason: %s", filepath, buf);
    #endif

    fflush (get_log_stream ());
    return counter;
}

// -------------------------------------------------------------------------------------------------

tree::node_t *tree::new_node ()
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    const tree::node_t default_node = {};
    memcpy (node, &default_node, sizeof (tree::node_t));

    node->type    = node_type_t::NOT_SET;   

    return node;
}

tree::node_t *tree::new_node (node_type_t type, int data)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type = type;
    node->data = data;    

    return node;
}

tree::node_t *tree::new_node (node_type_t type, op_t op)
{
    return new_node(type, (int) op);
}

tree::node_t *tree::new_node (node_type_t type, int data, node_t *left, node_t *right)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type  = type;
    node->data  = data;   
    node->left  = left;
    node->right = right; 

    return node;
}

tree::node_t *tree::new_node (node_type_t type, op_t op, node_t *left, node_t *right)
{
    return new_node(type, (int) op, left, right);
}

// -------------------------------------------------------------------------------------------------

void tree::del_node (node_t *start_node)
{
    if (start_node == nullptr)
    {
        return;
    }

    tree::walk_f free_node_func = [](node_t* node, void *, bool){ free(node); return true; };

    dfs_recursion (start_node, nullptr,        nullptr,
                               nullptr,        nullptr,
                               free_node_func, nullptr);
}

void tree::del_left  (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->left);
    node->left = nullptr;
}

void tree::del_right (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->right);
    node->right = nullptr;
}

void tree::del_childs (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->right);
    del_node (node->left);
    node->right = nullptr;
    node->left  = nullptr;
}

// -------------------------------------------------------------------------------------------------
// PRIVATE SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param)
{
    assert (node != nullptr && "invalid pointer");

    bool cont = true; 

    if (pre_exec != nullptr)
    {
        cont = pre_exec (node, pre_param, cont) && cont;
    }

    if (cont && node->left != nullptr)
    {
        cont = cont && dfs_recursion (node->left, pre_exec,  pre_param,
                                                  in_exec,   in_param,
                                                  post_exec, post_param);
    }

    if (in_exec != nullptr)
    {
        cont = in_exec (node, in_param, cont) && cont;

    }

    if (cont && node->right != nullptr)
    {
        cont = cont && dfs_recursion (node->right, pre_exec,  pre_param,
                                    in_exec,   in_param,
                                    post_exec, post_param);
    }

    if (post_exec != nullptr)
    {
        cont = post_exec (node, post_param, cont) && cont;
    }

    return cont;
}

// -------------------------------------------------------------------------------------------------

static bool node_codegen (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream = (FILE *) stream_void;
    char buf[MAX_NODE_LEN] = "";
    format_node (buf, node);

    fprintf (stream, "node_%p [label = \"%s | {l: %p | r: %p}\"]\n",
                                                        node, buf,
                                                        node->left, node->right);

    if (node->left != nullptr)
    {
        fprintf (stream, "node_%p -> node_%p\n", node, node->left);
    }

    if (node->right != nullptr)
    {
        fprintf (stream, "node_%p -> node_%p\n", node, node->right);
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

#define _PRINT(fmt, ...)                \
{                                       \
    sprintf (buf, fmt, ##__VA_ARGS__);  \
    return;                             \
}

static void format_node (char *buf, const tree::node_t *node) 
{
    assert (buf  != nullptr && "invalid pointer");
    assert (node != nullptr && "invalid pointer");

    switch (node->type)
    {
        case tree::node_type_t::NOT_SET:    _PRINT ("NOT SET");
        case tree::node_type_t::FICTIOUS:   _PRINT ("FICTIOUS");
        case tree::node_type_t::VAL:        _PRINT ("VAL: %d",  node->data);
        case tree::node_type_t::VAR:        _PRINT ("VAR: #%d", node->data);
        case tree::node_type_t::IF:         _PRINT ("IF");
        case tree::node_type_t::ELSE:       _PRINT ("ELSE");
        case tree::node_type_t::WHILE:      _PRINT ("WHILE");
        case tree::node_type_t::OP:         _PRINT ("OP: %s", get_op_name ((tree::op_t) node->data));
        case tree::node_type_t::VAR_DEF:    _PRINT ("VAR DEF: #%d",   node->data);
        case tree::node_type_t::FUNC_DEF:   _PRINT ("FUNC_DEF: #%d",  node->data);
        case tree::node_type_t::FUNC_CALL:  _PRINT ("FUNC_CALL: #%d", node->data);
        case tree::node_type_t::RETURN:     _PRINT ("RETURN");
        
        default:
            assert (0 && "Unexpected node type");
    }
}

// -------------------------------------------------------------------------------------------------

#define _OP(op, name)      \
    case tree::op_t::op:   \
        return name;       \

static const char *get_op_name (tree::op_t op)
{
    switch (op) {
        _OP(ADD,    "+")
        _OP(SUB,    "-")
        _OP(DIV,    "/")
        _OP(MUL,    "*")
        _OP(POW,    "pow")
        _OP(INPUT,  "INP")
        _OP(OUTPUT, "OUT")
        _OP(EQ,     "==")
        _OP(GT,     ">")
        _OP(LT,     "<")
        _OP(GE,     ">=") 
        _OP(LE,     "<=") 
        _OP(NEQ,    "!=") 
        _OP(NOT,    "NOT")
        _OP(AND,    "&&") 
        _OP(OR,     "||") 
        _OP(ASSIG,  ":=") 

        default:
            assert (0 && "Invalid op, possible union error");
    }
}