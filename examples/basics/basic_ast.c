#include <bml/bml.h>
#include <bml/core/ast_print.h>

int main(void)
{
  BML_Context *ctx = bml_context_create();

  BML_Expr *x = bml_variable(ctx, "x");

  BML_Expr *five = bml_number(ctx, 5);

  BML_Expr *expr = bml_binary(ctx, BML_NODE_ADD, x, five);

  bml_print_ast(expr);

  bml_context_destroy(ctx);

  return 0;
}
