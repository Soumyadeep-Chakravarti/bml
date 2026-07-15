#include "bml/core/ast.h"
#include "bml/core/context.h"

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
