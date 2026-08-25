#include "bml/core/ast_print.h"

#include <stdio.h>

static const char *bml_node_name(BML_NodeType type) {
  switch (type) {
  case BML_NODE_NUMBER:
    return "NUMBER";

  case BML_NODE_VARIABLE:
    return "VARIABLE";

  case BML_NODE_CONSTANT:
    return "CONSTANT";

  case BML_NODE_NEGATE:
    return "NEGATE";

  case BML_NODE_ADD:
    return "ADD";

  case BML_NODE_SUBTRACT:
    return "SUBTRACT";

  case BML_NODE_MULTIPLY:
    return "MULTIPLY";

  case BML_NODE_DIVIDE:
    return "DIVIDE";

  case BML_NODE_POWER:
    return "POWER";

  case BML_NODE_FUNCTION:
    return "FUNCTION";

  case BML_NODE_EQUATION:
    return "EQUATION";

  case BML_NODE_DERIVATIVE:
    return "DERIVATIVE";

  case BML_NODE_INTEGRAL:
    return "INTEGRAL";

  case BML_NODE_LIMIT:
    return "LIMIT";

  case BML_NODE_VECTOR:
    return "VECTOR";

  case BML_NODE_MATRIX:
    return "MATRIX";

  case BML_NODE_PIECEWISE:
    return "PIECEWISE";

  default:
    return "UNKNOWN";
  }
}

static void print_indent(size_t depth) {
  for (size_t i = 0; i < depth; i++) {
    printf("    ");
  }
}

static void print_node(const BML_Expr *expr, size_t depth) {
  if (!expr)
    return;

  print_indent(depth);

  printf("%s", bml_node_name(expr->type));

  switch (expr->type) {
  case BML_NODE_NUMBER:
    printf(" (%f)", expr->value.number);
    break;

  case BML_NODE_VARIABLE:
  case BML_NODE_CONSTANT:
    printf(" (%s)", expr->value.identifier);
    break;

  default:
    break;
  }

  printf("\n");

  for (size_t i = 0; i < expr->child_count; i++) {
    print_node(expr->children[i], depth + 1);
  }
}

void bml_print_ast(const BML_Expr *expr) {
  print_node(expr, 0);
}
