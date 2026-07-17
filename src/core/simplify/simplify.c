#include "bml/core/simplify.h"
#include "bml/core/ast.h"
#include "bml/core/ast_builder.h"
#include "bml/core/context.h"
#include "bml/internal/macros.h"
#include "rules/rules.h"

typedef BML_Expr *(*BML_RuleFn)(BML_Context *ctx, const BML_Expr *left,
                                const BML_Expr *right);

typedef BML_Expr *(*BML_UnaryRuleFn)(BML_Context *ctx, const BML_Expr *child);

static BML_Expr *apply_rules(BML_Context *ctx, const BML_Expr *left,
                             const BML_Expr *right, const BML_RuleFn *rules,
                             size_t count) {
  for (size_t i = 0; i < count; i++) {
    BML_Expr *result = rules[i](ctx, left, right);
    if (result)
      return result;
  }
  return NULL;
}

static BML_Expr *apply_unary_rules(BML_Context *ctx, const BML_Expr *child,
                                   const BML_UnaryRuleFn *rules, size_t count) {
  for (size_t i = 0; i < count; i++) {
    BML_Expr *result = rules[i](ctx, child);
    if (result)
      return result;
  }
  return NULL;
}

static BML_Expr *rebuild_binary(BML_Context *ctx, BML_NodeType type,
                                BML_Expr *left, BML_Expr *right) {
  switch (type) {
  case BML_NODE_ADD:
    return bml_add(ctx, left, right);
  case BML_NODE_SUBTRACT:
    return bml_subtract(ctx, left, right);
  case BML_NODE_MULTIPLY:
    return bml_multiply(ctx, left, right);
  case BML_NODE_DIVIDE:
    return bml_divide(ctx, left, right);
  case BML_NODE_POWER:
    return bml_power(ctx, left, right);
  default:
    return NULL;
  }
}

static BML_Expr *simplify_binary(BML_Context *ctx, const BML_Expr *expr,
                                 const BML_RuleFn *rules, size_t rule_count) {
  BML_Expr *left = bml_simplify(ctx, expr->children[0]);
  BML_Expr *right = bml_simplify(ctx, expr->children[1]);

  int left_changed = (left != expr->children[0]);
  int right_changed = (right != expr->children[1]);

  BML_Expr *result = apply_rules(ctx, left, right, rules, rule_count);
  if (result)
    return result;

  if (left_changed || right_changed)
    return rebuild_binary(ctx, expr->type, left, right);

  return (BML_Expr *)expr;
}

/*==========================================================
 * Rule Tables (static, one per operator)
 *==========================================================*/

static const BML_RuleFn add_rules[] = {
    rule_add_zero,
    rule_add_self,
    rule_fold_add,
};

static const BML_RuleFn sub_rules[] = {
    rule_sub_self,
    rule_sub_zero,
    rule_fold_sub,
};

static const BML_UnaryRuleFn negate_rules[] = {
    rule_cancel_negation,
};

static const BML_RuleFn mul_rules[] = {
    rule_mul_zero,
    rule_mul_one,
    rule_fold_mul,
};

static const BML_RuleFn div_rules[] = {
    rule_div_one,
    rule_fold_div,
};

static const BML_RuleFn pow_rules[] = {
    rule_pow_zero,
    rule_pow_one,
    rule_one_pow,
    rule_fold_pow,
};

BML_Expr *bml_simplify(BML_Context *ctx, const BML_Expr *expr) {
  if (!expr)
    return NULL;

  switch (expr->type) {
  case BML_NODE_NUMBER:
  case BML_NODE_VARIABLE:
  case BML_NODE_CONSTANT:
    return (BML_Expr *)expr;

  case BML_NODE_ADD:
    return simplify_binary(ctx, expr, add_rules, BML_ARRAY_SIZE(add_rules));

  case BML_NODE_SUBTRACT:
    return simplify_binary(ctx, expr, sub_rules, BML_ARRAY_SIZE(sub_rules));

  case BML_NODE_MULTIPLY:
    return simplify_binary(ctx, expr, mul_rules, BML_ARRAY_SIZE(mul_rules));

  case BML_NODE_DIVIDE:
    return simplify_binary(ctx, expr, div_rules, BML_ARRAY_SIZE(div_rules));

  case BML_NODE_POWER:
    return simplify_binary(ctx, expr, pow_rules, BML_ARRAY_SIZE(pow_rules));

  case BML_NODE_NEGATE: {
    BML_Expr *child = bml_simplify(ctx, expr->children[0]);

    BML_Expr *result = apply_unary_rules(ctx, child, negate_rules,
                                         BML_ARRAY_SIZE(negate_rules));
    if (result)
      return result;

    if (child != expr->children[0])
      return bml_negate(ctx, child);

    return (BML_Expr *)expr;
  }

  default:
    return (BML_Expr *)expr;
  }
}
