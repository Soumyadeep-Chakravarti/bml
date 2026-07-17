#ifndef BML_INTERNAL_EXPR_UTILS_H
#define BML_INTERNAL_EXPR_UTILS_H

#include "bml/core/ast.h"

static inline int bml_is_number(const BML_Expr *expr) {
  return expr && expr->type == BML_NODE_NUMBER;
}

static inline int bml_is_zero(const BML_Expr *expr) {
  return bml_is_number(expr) && expr->value.number == 0.0;
}

static inline int bml_is_one(const BML_Expr *expr) {
  return bml_is_number(expr) && expr->value.number == 1.0;
}

static inline int bml_is_negate(const BML_Expr *expr) {
  return expr && expr->type == BML_NODE_NEGATE;
}

#endif /* BML_INTERNAL_EXPR_UTILS_H */
