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

static void test_semantics(BML_Context *ctx) {
  const char *inputs[] = {"(x+y)^3 + sin(x)", "(x*x + y) / 2", "exp(x) + y^2"};
  BML_Binding bindings[] = {{"x", 0.25}, {"y", 1.5}};
  for (size_t i = 0; i < sizeof(inputs) / sizeof(*inputs); i++) {
    BML_Error error;
    double input, simplified, canonical, optimized;
    BML_Expr *expr = bml_parse(ctx, inputs[i], &error);
    BML_OptimizerResult result;
    CHECK(expr && error == BML_SUCCESS);
    CHECK(bml_evaluate(expr, bindings, 2, &input) == BML_SUCCESS);
    CHECK(bml_evaluate(bml_simplify(ctx, expr), bindings, 2, &simplified) ==
          BML_SUCCESS);
    CHECK(bml_evaluate(bml_canonicalize(ctx, expr), bindings, 2, &canonical) ==
          BML_SUCCESS);
    CHECK(bml_optimize(ctx, expr, BML_OPTIMIZE_OPERATION_COUNT, &result));
    CHECK(bml_evaluate(result.selected, bindings, 2, &optimized) == BML_SUCCESS);
    CHECK(fabs(input - simplified) < 1e-12 && fabs(input - canonical) < 1e-12 &&
          fabs(input - optimized) < 1e-12);
  }
}

static void test_deterministic_fuzz(BML_Context *ctx) {
  static const char alphabet[] = "xy012+*/^()- sin";
  unsigned state = 0x0c0ffeeu;
  for (size_t i = 0; i < 1000; i++) {
    char text[33];
    for (size_t j = 0; j < sizeof(text) - 1; j++) {
      state = state * 1664525u + 1013904223u;
      text[j] = alphabet[(state >> 16) % (sizeof(alphabet) - 1)];
    }
    text[sizeof(text) - 1] = '\0';
    BML_Error error;
    BML_Expr *expr = bml_parse(ctx, text, &error);
    if (expr) {
      BML_Expr *first = bml_simplify(ctx, expr);
      BML_Expr *second = bml_simplify(ctx, expr);
      CHECK(first && second && bml_expr_equal(first, second));
    } else
      CHECK(error == BML_ERROR_PARSE || error == BML_ERROR_OUT_OF_MEMORY);
  }
}

int main(void) {
  BML_Context *ctx = bml_context_create();
  CHECK(ctx);
  if (ctx) {
    test_semantics(ctx);
    test_deterministic_fuzz(ctx);
    bml_context_destroy(ctx);
  }
  return failures != 0;
}
