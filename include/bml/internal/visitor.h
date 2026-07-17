#ifndef BML_INTERNAL_VISITOR_H
#define BML_INTERNAL_VISITOR_H

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
  BML_VISIT_PREORDER,
  BML_VISIT_POSTORDER
} BML_VisitOrder;

/*==========================================================
 * Generic AST Walker
 *==========================================================*/

/* Walks the expression tree depth-first.
 * Calls fn(node, userdata) for each node in the specified order.
 * Returns the first non-zero value from fn, or 0 if traversal completes. */
int bml_visit(const BML_Expr *expr, BML_VisitFn fn, BML_VisitOrder order,
              void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* BML_INTERNAL_VISITOR_H */
