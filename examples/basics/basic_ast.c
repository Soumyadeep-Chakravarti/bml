#include <stdio.h>

#include <bml/bml.h>
#include <bml/core/ast_print.h>

static void demo(BML_Context *ctx, const char *label, BML_Expr *expr) {
  printf("%s:\n", label);
  printf("  before: ");
  bml_print_ast(expr);
  expr = bml_simplify(ctx, expr);
  printf("  after:  ");
  bml_print_ast(expr);
  printf("\n");
}

static void demo_eq(const char *label, const BML_Expr *a, const BML_Expr *b) {
  printf("%s: %s\n", label, bml_expr_equal(a, b) ? "equal" : "not equal");
}

int main(void) {
  BML_Context *ctx = bml_context_create();

  /* addition */
  demo(ctx, "x + 0", bml_add(ctx, bml_variable(ctx, "x"), bml_number(ctx, 0)));
  demo(ctx, "0 + x", bml_add(ctx, bml_number(ctx, 0), bml_variable(ctx, "x")));
  demo(ctx, "2 + 3", bml_add(ctx, bml_number(ctx, 2), bml_number(ctx, 3)));
  demo(ctx, "x + x", bml_add(ctx, bml_variable(ctx, "x"), bml_variable(ctx, "x")));

  /* subtraction */
  demo(ctx, "x - 0",
       bml_subtract(ctx, bml_variable(ctx, "x"), bml_number(ctx, 0)));
  demo(ctx, "0 - x",
       bml_subtract(ctx, bml_number(ctx, 0), bml_variable(ctx, "x")));
  demo(ctx, "5 - 3",
       bml_subtract(ctx, bml_number(ctx, 5), bml_number(ctx, 3)));
  demo(ctx, "x - x",
       bml_subtract(ctx, bml_variable(ctx, "x"), bml_variable(ctx, "x")));

  /* negation */
  demo(ctx, "-( -x )",
       bml_negate(ctx, bml_negate(ctx, bml_variable(ctx, "x"))));

  /* multiplication */
  demo(ctx, "x * 1",
       bml_multiply(ctx, bml_variable(ctx, "x"), bml_number(ctx, 1)));
  demo(ctx, "1 * x",
       bml_multiply(ctx, bml_number(ctx, 1), bml_variable(ctx, "x")));
  demo(ctx, "x * 0",
       bml_multiply(ctx, bml_variable(ctx, "x"), bml_number(ctx, 0)));
  demo(ctx, "4 * 7",
       bml_multiply(ctx, bml_number(ctx, 4), bml_number(ctx, 7)));

  /* division */
  demo(ctx, "x / 1",
       bml_divide(ctx, bml_variable(ctx, "x"), bml_number(ctx, 1)));
  demo(ctx, "0 / x",
       bml_divide(ctx, bml_number(ctx, 0), bml_variable(ctx, "x")));
  demo(ctx, "10 / 2",
       bml_divide(ctx, bml_number(ctx, 10), bml_number(ctx, 2)));

  /* power */
  demo(ctx, "x ^ 0",
       bml_power(ctx, bml_variable(ctx, "x"), bml_number(ctx, 0)));
  demo(ctx, "x ^ 1",
       bml_power(ctx, bml_variable(ctx, "x"), bml_number(ctx, 1)));
  demo(ctx, "1 ^ x",
       bml_power(ctx, bml_number(ctx, 1), bml_variable(ctx, "x")));
  demo(ctx, "0 ^ x",
       bml_power(ctx, bml_number(ctx, 0), bml_variable(ctx, "x")));
  demo(ctx, "2 ^ 3",
       bml_power(ctx, bml_number(ctx, 2), bml_number(ctx, 3)));

  /* structural equality */
  printf("--- structural equality ---\n");
  BML_Expr *x = bml_variable(ctx, "x");
  BML_Expr *y = bml_variable(ctx, "y");
  BML_Expr *x2 = bml_variable(ctx, "x");

  demo_eq("x == x", x, x2);
  demo_eq("x == y", x, y);
  demo_eq("x+1 == x+1", bml_add(ctx, x, bml_number(ctx, 1)),
          bml_add(ctx, x2, bml_number(ctx, 1)));
  demo_eq("x+1 == x+2", bml_add(ctx, x, bml_number(ctx, 1)),
          bml_add(ctx, x, bml_number(ctx, 2)));

  bml_context_destroy(ctx);

  return 0;
}
