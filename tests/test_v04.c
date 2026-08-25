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

static BML_Expr *parse(BML_Context *ctx, const char *text) {
  BML_Error error;
  BML_Expr *expr = bml_parse(ctx, text, &error);
  CHECK(expr && error == BML_SUCCESS);
  return expr;
}

static void test_semantics(BML_Context *ctx) {
  CHECK(bml_context_assume(ctx, "p", BML_ASSUME_POSITIVE));
  uint32_t facts = bml_context_assumptions(ctx, "p");
  CHECK((facts & (BML_ASSUME_POSITIVE | BML_ASSUME_NONZERO | BML_ASSUME_NONNEGATIVE)) ==
        (BML_ASSUME_POSITIVE | BML_ASSUME_NONZERO | BML_ASSUME_NONNEGATIVE));
  CHECK(bml_expr_definedness(ctx, parse(ctx, "1/p")) == BML_DEFINED);
  CHECK(bml_expr_definedness(ctx, parse(ctx, "log(p)")) == BML_DEFINED);
  CHECK(bml_expr_definedness(ctx, parse(ctx, "x^0")) == BML_DEFINED_UNKNOWN);
  CHECK(bml_context_assume(ctx, "x", BML_ASSUME_NONZERO));
  CHECK(bml_expr_is_defined(ctx, parse(ctx, "x^0")));
  CHECK(bml_expr_definedness(ctx, parse(ctx, "0*log(y)")) == BML_DEFINED_UNKNOWN);
}

static void test_verification(BML_Context *ctx) {
  BML_VerificationResult result;
  BML_VerifyOptions generated = {NULL, 0, 12, 1e-9};
  CHECK(bml_verify_equivalent(ctx, parse(ctx, "(x+y)^2 - 2*x*y"), parse(ctx, "x^2+y^2"),
                              &generated, &result));
  CHECK(result.trials == 12 && result.passed == 12 && result.first_failure == SIZE_MAX);
  BML_Binding bindings[] = {{"x", 2.0}, {"y", -0.5}};
  BML_VerifyOptions supplied = {bindings, 2, 0, 1e-9};
  CHECK(bml_verify_equivalent(ctx, parse(ctx, "x+y"), parse(ctx, "x-y"), &supplied,
                              &result) == 0);
  CHECK(result.first_failure == 0 && result.passed == 0);
  BML_Expr *expr = parse(ctx, "sin(x) + x^3");
  BML_Expr *derivative = bml_simplify(ctx, bml_differentiate(ctx, expr, "x"));
  BML_VerifyOptions derivative_options = {NULL, 0, 12, 1e-5};
  CHECK(
      bml_verify_derivative(ctx, expr, derivative, "x", &derivative_options, &result));
  CHECK(result.trials == 12 && result.passed == 12);
}

static void test_egraph(BML_Context *ctx) {
  BML_OptimizerResult result;
  CHECK(bml_optimize_with_engine(ctx, parse(ctx, "(x+y)^2 - 2*x*y"),
                                 BML_OPTIMIZE_READABILITY, BML_OPTIMIZER_EGRAPH,
                                 &result));
  CHECK(result.engine == BML_OPTIMIZER_EGRAPH && result.egraph.generated == 1);
  CHECK(result.proof_steps && strstr(result.proof_steps, "binomial-square"));
  CHECK(result.selected_candidate == 2);
  CHECK(bml_optimize_with_engine(ctx, parse(ctx, "(x+y)^2 - 2*x*y"),
                                 BML_OPTIMIZE_OPERATION_COUNT, BML_OPTIMIZER_EGRAPH,
                                 &result));
  CHECK(result.selected_candidate == 2);
  CHECK(result.candidates[2].cost.mathematical_operations == 3);
  CHECK(result.candidates[1].cost.mathematical_operations == 6);
}

int main(void) {
  BML_Context *ctx = bml_context_create();
  CHECK(ctx);
  if (ctx) {
    test_semantics(ctx);
    test_verification(ctx);
    test_egraph(ctx);
    bml_context_destroy(ctx);
  }
  return failures != 0;
}
