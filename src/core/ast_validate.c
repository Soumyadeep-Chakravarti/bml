#include "bml/core/ast_validate.h"

static BML_ValidationError validate_node(const BML_Expr *expr) {
  if (!expr)
    return BML_INVALID_NULL_NODE;

  switch (expr->type) {
  case BML_NODE_NUMBER:
  case BML_NODE_VARIABLE:
  case BML_NODE_CONSTANT:

    if (expr->child_count != 0)
      return BML_INVALID_CHILD_COUNT;

    break;

  case BML_NODE_NEGATE:

    if (expr->child_count != 1)
      return BML_INVALID_CHILD_COUNT;

    break;

  case BML_NODE_ADD:
  case BML_NODE_SUBTRACT:
  case BML_NODE_MULTIPLY:
  case BML_NODE_DIVIDE:
  case BML_NODE_POWER:

    if (expr->child_count != 2)
      return BML_INVALID_CHILD_COUNT;

    break;

  default:
    break;
  }

  for (size_t i = 0; i < expr->child_count; i++) {
    BML_ValidationError result = validate_node(expr->children[i]);

    if (result != BML_VALID)
      return result;
  }

  return BML_VALID;
}

BML_ValidationError bml_validate_ast(const BML_Expr *expr) {
  return validate_node(expr);
}

const char *bml_validation_error_string(BML_ValidationError error) {
  switch (error) {
  case BML_VALID:
    return "Valid AST";

  case BML_INVALID_NULL_NODE:
    return "Null node";

  case BML_INVALID_CHILD_COUNT:
    return "Invalid child count";

  case BML_INVALID_NODE_TYPE:
    return "Invalid node type";

  default:
    return "Unknown validation error";
  }
}
