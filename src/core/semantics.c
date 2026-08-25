#include "bml/semantics.h"

#include <math.h>
#include <string.h>

uint32_t bml_expr_assumptions(const BML_Context *ctx, const BML_Expr *expr) {
  if (!expr)
    return BML_ASSUME_NONE;
  if (expr->type == BML_NODE_VARIABLE)
    return bml_context_assumptions(ctx, expr->value.identifier);
  if (expr->type == BML_NODE_NUMBER) {
    uint32_t facts =
        isfinite(expr->value.number) ? BML_ASSUME_REAL | BML_ASSUME_FINITE : 0;
    if (expr->value.number > 0.0)
      facts |= BML_ASSUME_POSITIVE | BML_ASSUME_NONNEGATIVE | BML_ASSUME_NONZERO;
    else if (expr->value.number == 0.0)
      facts |= BML_ASSUME_NONNEGATIVE | BML_ASSUME_INTEGER;
    else
      facts |= BML_ASSUME_NONZERO;
    return facts;
  }
  if (expr->type == BML_NODE_NEGATE) {
    uint32_t facts = bml_expr_assumptions(ctx, expr->children[0]);
    return facts & (BML_ASSUME_REAL | BML_ASSUME_FINITE | BML_ASSUME_NONZERO);
  }
  if (expr->type == BML_NODE_POWER && expr->children[1]->type == BML_NODE_NUMBER &&
      floor(expr->children[1]->value.number) == expr->children[1]->value.number &&
      ((long)expr->children[1]->value.number % 2L == 0L))
    return (bml_expr_assumptions(ctx, expr->children[0]) & BML_ASSUME_FINITE)
               ? BML_ASSUME_FINITE | BML_ASSUME_NONNEGATIVE
               : BML_ASSUME_NONNEGATIVE;
  return BML_ASSUME_NONE;
}

static int known_positive(const BML_Context *ctx, const BML_Expr *expr) {
  return bml_expr_assumptions(ctx, expr) & BML_ASSUME_POSITIVE;
}

BML_Definedness bml_expr_definedness(const BML_Context *ctx, const BML_Expr *expr) {
  if (!expr)
    return BML_UNDEFINED;
  if (expr->type == BML_NODE_NUMBER)
    return isfinite(expr->value.number) ? BML_DEFINED : BML_UNDEFINED;
  if (expr->type == BML_NODE_VARIABLE)
    return BML_DEFINED;
  int unknown_child = 0;
  for (size_t i = 0; i < expr->child_count; i++) {
    BML_Definedness child = bml_expr_definedness(ctx, expr->children[i]);
    if (child == BML_UNDEFINED)
      return BML_UNDEFINED;
    if (child == BML_DEFINED_UNKNOWN)
      unknown_child = 1;
  }
  if (expr->type == BML_NODE_DIVIDE)
    return !unknown_child &&
                   (bml_expr_assumptions(ctx, expr->children[1]) & BML_ASSUME_NONZERO)
               ? BML_DEFINED
               : BML_DEFINED_UNKNOWN;
  if (expr->type == BML_NODE_FUNCTION && !strcmp(expr->value.identifier, "log"))
    return !unknown_child && known_positive(ctx, expr->children[0])
               ? BML_DEFINED
               : BML_DEFINED_UNKNOWN;
  if (expr->type == BML_NODE_POWER) {
    const BML_Expr *base = expr->children[0], *exponent = expr->children[1];
    if (exponent->type == BML_NODE_NUMBER && exponent->value.number == 0.0 &&
        !(bml_expr_assumptions(ctx, base) & BML_ASSUME_NONZERO))
      return BML_DEFINED_UNKNOWN;
    if (base->type == BML_NODE_NUMBER && exponent->type == BML_NODE_NUMBER &&
        !isfinite(pow(base->value.number, exponent->value.number)))
      return BML_UNDEFINED;
    if (!unknown_child && known_positive(ctx, base))
      return BML_DEFINED;
    if (exponent->type == BML_NODE_NUMBER &&
        floor(exponent->value.number) == exponent->value.number) {
      return unknown_child ? BML_DEFINED_UNKNOWN : BML_DEFINED;
    }
    return BML_DEFINED_UNKNOWN;
  }
  return unknown_child ? BML_DEFINED_UNKNOWN : BML_DEFINED;
}

int bml_expr_is_defined(const BML_Context *ctx, const BML_Expr *expr) {
  return bml_expr_definedness(ctx, expr) == BML_DEFINED;
}
