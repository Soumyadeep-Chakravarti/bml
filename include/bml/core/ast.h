typedef struct BML_Context BML_Context;

#ifndef BML_AST_H
#define BML_AST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================
 * Node Types
 *=========================================w=================*/

typedef enum {
  /* Literals */
  BML_NODE_NUMBER,
  BML_NODE_VARIABLE,
  BML_NODE_CONSTANT,

  /* Unary Operators */
  BML_NODE_NEGATE,

  /* Binary Operators */
  BML_NODE_ADD,
  BML_NODE_SUBTRACT,
  BML_NODE_MULTIPLY,
  BML_NODE_DIVIDE,
  BML_NODE_POWER,

  /* Generic Function */
  BML_NODE_FUNCTION,

  /* Relations */
  BML_NODE_EQUATION,

  /* Calculus */
  BML_NODE_DERIVATIVE,
  BML_NODE_INTEGRAL,
  BML_NODE_LIMIT,

  /* Containers */
  BML_NODE_VECTOR,
  BML_NODE_MATRIX,
  BML_NODE_PIECEWISE,

  /* Placeholder */
  BML_NODE_UNKNOWN

} BML_NodeType;

/*==========================================================
 * Forward Declaration
 *==========================================================*/

typedef struct BML_Expr BML_Expr;

/*==========================================================
 * Expression Node
 *==========================================================*/

struct BML_Expr {
  BML_NodeType type;

  union {
    double number;

    const char *identifier;
  } value;

  size_t child_count;

  BML_Expr **children;
};

/*==========================================================
 * Destruction
 *==========================================================*/

void bml_expr_destroy(BML_Expr *expr);

/*==========================================================
 * Debug
 *==========================================================*/

void bml_print_ast(const BML_Expr *expr);

/*==========================================================
 * Comparison
 *==========================================================*/

/* Structural equality: returns 1 if both expression trees are
 * identical in structure and values. Does not canonicalize
 * (so x+y != y+x). Use for rewrite rule matching only. */
int bml_expr_equal(const BML_Expr *a, const BML_Expr *b);

#ifdef __cplusplus
}
#endif

#endif /* BML_AST_H */
