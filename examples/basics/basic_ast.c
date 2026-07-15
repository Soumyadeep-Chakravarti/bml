#include <stdio.h>

#include <bml/bml.h>
#include <bml/core/ast_print.h>
#include <bml/core/ast_validate.h>

int main(void) {
  BML_Context *ctx = bml_context_create();

  BML_Expr *x = bml_variable(ctx, "x");

  BML_Expr *five = bml_number(ctx, 5);

  BML_Expr *expr = bml_binary(ctx, BML_NODE_ADD, x, five);

  bml_print_ast(expr);

  BML_ValidationError result = bml_validate_ast(expr);

  printf("%s\n", bml_validation_error_string(result));

  bml_context_destroy(ctx);

  return 0;
}
