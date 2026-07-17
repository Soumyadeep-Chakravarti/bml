#include "rules.h"

#include <math.h>

#include "bml/core/ast.h"
#include "bml/core/ast_builder.h"
#include "bml/core/context.h"
#include "bml/internal/expr_utils.h"

/*==========================================================
 * Addition Rules
 *==========================================================*/

BML_Expr *rule_add_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  (void)ctx;

  if (bml_is_zero(right))
    return (BML_Expr *)left;

  if (bml_is_zero(left))
    return (BML_Expr *)right;

  return NULL;
}

BML_Expr *rule_add_self(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_expr_equal(left, right)) {
    BML_Expr *two = bml_number(ctx, 2.0);
    return bml_multiply(ctx, two, (BML_Expr *)left);
  }

  return NULL;
}

BML_Expr *rule_fold_add(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_number(left) && bml_is_number(right))
    return bml_number(ctx, left->value.number + right->value.number);

  return NULL;
}

/*==========================================================
 * Subtraction Rules
 *==========================================================*/

BML_Expr *rule_sub_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_zero(right))
    return (BML_Expr *)left;

  if (bml_is_zero(left))
    return bml_negate(ctx, (BML_Expr *)right);

  return NULL;
}

BML_Expr *rule_sub_self(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_expr_equal(left, right))
    return bml_number(ctx, 0.0);

  return NULL;
}

BML_Expr *rule_fold_sub(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_number(left) && bml_is_number(right))
    return bml_number(ctx, left->value.number - right->value.number);

  return NULL;
}

/*==========================================================
 * Negation Rules
 *==========================================================*/

BML_Expr *rule_cancel_negation(BML_Context *ctx, const BML_Expr *left) {
  (void)ctx;

  if (bml_is_negate(left))
    return left->children[0];

  return NULL;
}

/*==========================================================
 * Multiplication Rules
 *==========================================================*/

BML_Expr *rule_mul_one(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right) {
  (void)ctx;

  if (bml_is_one(left))
    return (BML_Expr *)right;

  if (bml_is_one(right))
    return (BML_Expr *)left;

  return NULL;
}

BML_Expr *rule_mul_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_zero(left) || bml_is_zero(right))
    return bml_number(ctx, 0.0);

  return NULL;
}

BML_Expr *rule_fold_mul(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_number(left) && bml_is_number(right))
    return bml_number(ctx, left->value.number * right->value.number);

  return NULL;
}

/*==========================================================
 * Division Rules
 *==========================================================*/

BML_Expr *rule_div_one(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right) {
  (void)ctx;

  if (bml_is_one(right))
    return (BML_Expr *)left;

  if (bml_is_zero(left) && !bml_is_zero(right))
    return bml_number(ctx, 0.0);

  return NULL;
}

BML_Expr *rule_fold_div(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_number(left) && bml_is_number(right) && !bml_is_zero(right))
    return bml_number(ctx, left->value.number / right->value.number);

  return NULL;
}

/*==========================================================
 * Power Rules
 *==========================================================*/

BML_Expr *rule_pow_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  (void)left;

  if (bml_is_zero(right))
    return bml_number(ctx, 1.0);

  return NULL;
}

BML_Expr *rule_pow_one(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right) {
  (void)ctx;

  if (bml_is_one(right))
    return (BML_Expr *)left;

  return NULL;
}

BML_Expr *rule_one_pow(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right) {
  (void)right;

  if (bml_is_one(left))
    return bml_number(ctx, 1.0);

  return NULL;
}

BML_Expr *rule_fold_pow(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right) {
  if (bml_is_number(left) && bml_is_number(right)) {
    double value = pow(left->value.number, right->value.number);

    if (isfinite(value))
      return bml_number(ctx, value);
  }

  return NULL;
}
