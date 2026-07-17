#include "bml/internal/visitor.h"

static int visit_preorder(const BML_Expr *expr, BML_VisitFn fn,
                          void *userdata) {
  if (!expr)
    return 0;

  int rc = fn(expr, userdata);
  if (rc != 0)
    return rc;

  for (size_t i = 0; i < expr->child_count; i++) {
    rc = visit_preorder(expr->children[i], fn, userdata);
    if (rc != 0)
      return rc;
  }

  return 0;
}

static int visit_postorder(const BML_Expr *expr, BML_VisitFn fn,
                           void *userdata) {
  if (!expr)
    return 0;

  for (size_t i = 0; i < expr->child_count; i++) {
    int rc = visit_postorder(expr->children[i], fn, userdata);
    if (rc != 0)
      return rc;
  }

  return fn(expr, userdata);
}

int bml_visit(const BML_Expr *expr, BML_VisitFn fn, BML_VisitOrder order,
              void *userdata) {
  switch (order) {
  case BML_VISIT_POSTORDER:
    return visit_postorder(expr, fn, userdata);
  case BML_VISIT_PREORDER:
  default:
    return visit_preorder(expr, fn, userdata);
  }
}
