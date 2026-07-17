#include "bml/core/ast.h"
#include "bml/core/context.h"

#include <math.h>
#include <string.h>

BML_Expr *bml_node(BML_Context *ctx, BML_NodeType type, size_t child_count) {
  BML_Expr *expr = bml_arena_alloc(ctx->arena, sizeof(BML_Expr));

  if (!expr)
    return NULL;

  expr->type = type;
  expr->child_count = child_count;

  if (child_count > 0) {
    expr->children =
        bml_arena_alloc(ctx->arena, sizeof(BML_Expr *) * child_count);
  }

  return expr;
}

BML_Expr *bml_unary(BML_Context *ctx, BML_NodeType type, BML_Expr *child) {
  BML_Expr *expr = bml_node(ctx, type, 1);

  if (!expr)
    return NULL;

  expr->children[0] = child;

  return expr;
}

BML_Expr *bml_binary(BML_Context *ctx, BML_NodeType type, BML_Expr *left,
                     BML_Expr *right) {
  BML_Expr *expr = bml_node(ctx, type, 2);

  if (!expr)
    return NULL;

  expr->children[0] = left;
  expr->children[1] = right;

  return expr;
}

int bml_expr_equal(const BML_Expr *a, const BML_Expr *b) {
  if (a == b)
    return 1;

  if (!a || !b)
    return 0;

  if (a->type != b->type)
    return 0;

  switch (a->type) {
  case BML_NODE_NUMBER:
    /* Exact comparison is intentional.
     * Number nodes represent parsed literals rather than computed values,
     * so structural equality requires identical numeric values.
     */
    return a->value.number == b->value.number;

  case BML_NODE_VARIABLE:
  case BML_NODE_CONSTANT:
    return strcmp(a->value.identifier, b->value.identifier) == 0;

  default:
    break;
  }

  if (a->child_count != b->child_count)
    return 0;

  for (size_t i = 0; i < a->child_count; i++) {
    if (!bml_expr_equal(a->children[i], b->children[i]))
      return 0;
  }

  return 1;
}
