#include <math.h>
#include <stdio.h>
#include <string.h>

#include <bml/bml.h>

static int failures;
#define CHECK(x)                                                                       \
  do {                                                                                 \
    if (!(x)) {                                                                        \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                          \
      failures++;                                                                      \
    }                                                                                  \
  } while (0)

static void test_parse_eval(BML_Context *ctx) {
  BML_Error error;
  BML_Expr *expr = bml_parse(ctx, "sin(x) + 2 * y^2 - -3", &error);
  BML_Binding values[] = {{"x", 0.0}, {"y", 2.0}};
  double result;
  CHECK(expr && error == BML_SUCCESS);
  CHECK(bml_evaluate(expr, values, 2, &result) == BML_SUCCESS);
  CHECK(fabs(result - 11.0) < 1e-12);
  CHECK(!bml_parse(ctx, "sqrt(x)", &error) && error == BML_ERROR_PARSE);
  CHECK(!bml_parse(NULL, "x", &error) && error == BML_ERROR_INVALID_ARGUMENT);
  CHECK(bml_evaluate(expr, NULL, 1, &result) == BML_ERROR_INVALID_ARGUMENT);
}

static void test_transform_print_and_emit(BML_Context *ctx) {
  BML_Error error;
  char text[256];
  BML_Expr *expr = bml_parse(ctx, "x^3 + cos(x)", &error);
  BML_Expr *derivative = bml_simplify(ctx, bml_differentiate(ctx, expr, "x"));
  CHECK(derivative);
  CHECK(bml_format_infix(derivative, text, sizeof(text)) == BML_SUCCESS);
  CHECK(!strcmp(text, "3 * x ^ 2 + -sin(x)"));
  CHECK(bml_emit_c(expr, text, sizeof(text)) == BML_SUCCESS);
  CHECK(!strcmp(text, "(x * x * x) + cos(x)"));
  BML_Expr *replacement = bml_number(ctx, 2);
  BML_Expr *substituted = bml_substitute(ctx, expr, "x", replacement);
  CHECK(bml_evaluate(substituted, NULL, 0, &(double){0}) == BML_SUCCESS);
}

static void test_safe_rewrites(BML_Context *ctx) {
  BML_Error error;
  char text[64];
  CHECK(bml_format_infix(bml_simplify(ctx, bml_parse(ctx, "x - x", &error)), text,
                         sizeof(text)) == BML_SUCCESS);
  CHECK(!strcmp(text, "x - x"));
  CHECK(bml_format_infix(bml_simplify(ctx, bml_parse(ctx, "0 / x", &error)), text,
                         sizeof(text)) == BML_SUCCESS);
  CHECK(!strcmp(text, "0 / x"));
  CHECK(bml_format_infix(bml_simplify(ctx, bml_parse(ctx, "x ^ 0", &error)), text,
                         sizeof(text)) == BML_SUCCESS);
  CHECK(!strcmp(text, "x ^ 0"));
}

int main(void) {
  BML_Context *ctx = bml_context_create();
  CHECK(ctx);
  if (ctx) {
    test_parse_eval(ctx);
    test_transform_print_and_emit(ctx);
    test_safe_rewrites(ctx);
    bml_context_destroy(ctx);
  }
  return failures != 0;
}
