#include <bml/bml.h>
#include <bml/core/ast_print.h>

int main(void) {
  BML_Context *ctx = bml_context_create();

  BML_Expr *x = bml_variable(ctx, "x");

  BML_Expr *five = bml_number(ctx, 5);

  bml_print_ast(x);
  bml_print_ast(five);

  bml_context_destroy(ctx);

  return 0;
}
