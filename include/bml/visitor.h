#ifndef BML_VISITOR_H
#define BML_VISITOR_H

#include "bml/core/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================
 * Visitor Callback
 *==========================================================*/

/* Return values:
 *   0 = continue traversal
 *   non-zero = stop traversal, return that value from bml_visit */
typedef int (*BML_VisitFn)(const BML_Expr *expr, void *userdata);

/*==========================================================
 * Traversal Order
 *==========================================================*/

typedef enum {
  BML_TRAVERSE_PRE_ORDER,
  BML_TRAVERSE_POST_ORDER
} BML_TraversalOrder;

/*==========================================================
 * Generic AST Walker
 *==========================================================*/

/* Walks the expression tree depth-first in the specified order.
 * Calls fn(node, userdata) for each node.
 * Returns the first non-zero value from fn, or 0 if traversal completes. */
int bml_visit(const BML_Expr *root, BML_TraversalOrder order, BML_VisitFn fn,
              void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* BML_VISITOR_H */
