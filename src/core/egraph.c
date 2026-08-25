#include "bml/internal/egraph.h"

#include "bml/core/ast_builder.h"
#include "bml/internal/expr_utils.h"

/* This deliberately tiny experimental saturation set has one bounded identity. */
BML_Expr *bml_egraph_explore(BML_Context *ctx, const BML_Expr *expr,
                             BML_EGraphMetrics *metrics, const char **proof_steps) {
  *metrics = (BML_EGraphMetrics){1, 0, 64};
  *proof_steps = NULL;
  if (!expr || expr->type != BML_NODE_SUBTRACT ||
      expr->children[0]->type != BML_NODE_POWER ||
      !bml_is_number(expr->children[0]->children[1]) ||
      expr->children[0]->children[1]->value.number != 2.0 ||
      expr->children[0]->children[0]->type != BML_NODE_ADD)
    return NULL;
  const BML_Expr *a = expr->children[0]->children[0]->children[0];
  const BML_Expr *b = expr->children[0]->children[0]->children[1];
  const BML_Expr *term = expr->children[1];
  if (term->type != BML_NODE_MULTIPLY || term->children[0]->type != BML_NODE_MULTIPLY ||
      !bml_is_number(term->children[0]->children[0]) ||
      term->children[0]->children[0]->value.number != 2.0 ||
      !((bml_expr_equal(term->children[0]->children[1], a) &&
         bml_expr_equal(term->children[1], b)) ||
        (bml_expr_equal(term->children[0]->children[1], b) &&
         bml_expr_equal(term->children[1], a))))
    return NULL;
  metrics->generated = 1;
  *proof_steps = "egraph.binomial-square: (a + b)^2 - 2*a*b == a^2 + b^2";
  return bml_add(ctx, bml_power(ctx, (BML_Expr *)a, bml_number(ctx, 2.0)),
                 bml_power(ctx, (BML_Expr *)b, bml_number(ctx, 2.0)));
}
