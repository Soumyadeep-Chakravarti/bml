#include "bml/core/ast_builder.h"
#include "bml/core/context.h"
#include "bml/internal/ast_helpers.h"

/*==========================================================
 * Literals
 *==========================================================*/

BML_Expr *bml_number(BML_Context *ctx, double value) {
  BML_Expr *expr = bml_node(ctx, BML_NODE_NUMBER, 0);

  if (!expr)
    return NULL;

  expr->value.number = value;

  return expr;
}

BML_Expr *bml_variable(BML_Context *ctx, const char *name) {
  BML_Expr *expr = bml_node(ctx, BML_NODE_VARIABLE, 0);

  if (!expr)
    return NULL;

  expr->value.identifier = name;

  return expr;
}

BML_Expr *bml_constant(BML_Context *ctx, const char *name) {
  BML_Expr *expr = bml_node(ctx, BML_NODE_CONSTANT, 0);

  if (!expr)
    return NULL;

  expr->value.identifier = name;

  return expr;
}

/*==========================================================
 * Unary Operators
 *==========================================================*/

BML_Expr *bml_negate(BML_Context *ctx, BML_Expr *operand) {
  return bml_unary(ctx, BML_NODE_NEGATE, operand);
}

BML_Expr *bml_add(BML_Context *ctx, BML_Expr *left, BML_Expr *right) {
  return bml_binary(ctx, BML_NODE_ADD, left, right);
}

BML_Expr *bml_subtract(BML_Context *ctx, BML_Expr *left, BML_Expr *right) {
  return bml_binary(ctx, BML_NODE_SUBTRACT, left, right);
}

BML_Expr *bml_multiply(BML_Context *ctx, BML_Expr *left, BML_Expr *right) {
  return bml_binary(ctx, BML_NODE_MULTIPLY, left, right);
}

BML_Expr *bml_divide(BML_Context *ctx, BML_Expr *left, BML_Expr *right) {
  return bml_binary(ctx, BML_NODE_DIVIDE, left, right);
}

BML_Expr *bml_power(BML_Context *ctx, BML_Expr *base, BML_Expr *exponent) {
  return bml_binary(ctx, BML_NODE_POWER, base, exponent);
}
