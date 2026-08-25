#include <stdio.h>
#include <string.h>

#include <bml/bml.h>

static int failures;

#define CHECK(condition)                                                               \
  do {                                                                                 \
    if (!(condition)) {                                                                \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                  \
      failures++;                                                                      \
    }                                                                                  \
  } while (0)

static int record_visit(const BML_Expr *expr, void *userdata) {
  char *order = userdata;
  size_t length = strlen(order);
  order[length] = (char)('0' + expr->type);
  order[length + 1] = '\0';
  return 0;
}

static void test_arena_growth(BML_Context *ctx) {
  BML_Expr *first = bml_variable(ctx, "first");

  for (size_t i = 0; i < 512; i++)
    CHECK(bml_variable(ctx, "filler") != NULL);

  CHECK(bml_expr_equal(first, bml_variable(ctx, "first")));
}

static void test_simplification(BML_Context *ctx) {
  BML_Expr *expr =
      bml_multiply(ctx, bml_add(ctx, bml_number(ctx, 2.0), bml_number(ctx, 3.0)),
                   bml_number(ctx, 1.0));
  BML_Expr *simplified = bml_simplify(ctx, expr);

  CHECK(simplified->type == BML_NODE_NUMBER);
  CHECK(simplified->value.number == 5.0);
  CHECK(bml_expr_equal(bml_variable(ctx, "x"), bml_variable(ctx, "x")));
  CHECK(!bml_expr_equal(bml_variable(ctx, "x"), bml_variable(ctx, "y")));
}

typedef struct {
  size_t count;
  const char *id;
  const char *requirement;
} Trace;

static void record_trace(const BML_RewriteEvent *event, void *userdata) {
  Trace *trace = userdata;
  trace->count++;
  trace->id = event->id;
  trace->requirement = event->requirement;
}

static void test_compiler_core(BML_Context *ctx) {
  BML_Error error;
  char output[4096];
  BML_Expr *canonical = bml_parse(ctx, "2*x+x+0", &error);
  BML_Expr *expected = bml_parse(ctx, "3*x", &error);
  CHECK(bml_expr_equal(bml_canonicalize(ctx, canonical),
                       bml_canonicalize(ctx, expected)));
  canonical = bml_parse(ctx, "y+x", &error);
  expected = bml_parse(ctx, "x+y", &error);
  CHECK(bml_expr_equal(bml_canonicalize(ctx, canonical),
                       bml_canonicalize(ctx, expected)));
  canonical = bml_parse(ctx, "x*(y*z)", &error);
  expected = bml_parse(ctx, "x*y*z", &error);
  CHECK(bml_expr_equal(bml_canonicalize(ctx, canonical),
                       bml_canonicalize(ctx, expected)));

  BML_Expr *self_division = bml_parse(ctx, "x/x", &error);
  CHECK(bml_simplify(ctx, self_division)->type == BML_NODE_DIVIDE);
  CHECK(bml_context_assume(ctx, "x", BML_ASSUME_NONZERO));
  Trace trace = {0};
  CHECK(
      bml_simplify_with_options(ctx, self_division, NULL, record_trace, &trace)->type ==
      BML_NODE_NUMBER);
  CHECK(trace.count == 1 && !strcmp(trace.id, "bml.divide.self") &&
        !strcmp(trace.requirement, "nonzero"));

  BML_Expr *power = bml_parse(ctx, "x*x", &error);
  CHECK(bml_canonicalize(ctx, power)->type == BML_NODE_POWER);
  CHECK(bml_expr_hash(bml_canonicalize(ctx, canonical)) ==
        bml_expr_hash(bml_canonicalize(ctx, expected)));
  BML_OperationCost cost;
  bml_analyze_cost(power, &cost);
  CHECK(cost.multiplies == 1 && cost.estimated_c_operations == 1);

  BML_Expr *repeated = bml_parse(ctx, "(x+y)^2+sin(x+y)", &error);
  const char *parameters[] = {"x", "y"};
  CHECK(bml_emit_c_optimized(repeated, "f", parameters, 2, output, sizeof(output)) ==
        BML_SUCCESS);
  CHECK(strstr(output, "const double t0 = x + y;") != NULL);
  CHECK(strstr(output, "t0 * t0") != NULL);
  BML_Expr *raw = bml_parse(ctx, "2*x+x+0", &error);
  CHECK(bml_emit_c_optimized(raw, "f", parameters, 2, output, sizeof(output)) ==
        BML_SUCCESS);
  CHECK(strstr(output, "return 3 * x;") != NULL);
  CHECK(bml_emit_c_optimized(bml_parse(ctx, "x^2.5", &error), "f", parameters, 2,
                             output, sizeof(output)) == BML_SUCCESS);
  CHECK(strstr(output, "pow(x, 2.5)") != NULL);

  BML_OptimizerResult operation_result;
  BML_OptimizerResult readability_result;
  CHECK(bml_optimize(ctx, bml_parse(ctx, "x^3", &error), BML_OPTIMIZE_OPERATION_COUNT,
                     &operation_result));
  CHECK(!strcmp(operation_result.candidates[operation_result.selected_candidate].form,
                "power"));
  CHECK(bml_optimize(ctx, bml_parse(ctx, "x^3", &error), BML_OPTIMIZE_READABILITY,
                     &readability_result));
  CHECK(
      !strcmp(readability_result.candidates[readability_result.selected_candidate].form,
              "power"));
}

static void test_validation_and_visiting(BML_Context *ctx) {
  BML_Expr invalid = {
      .type = BML_NODE_NUMBER,
      .child_count = 1,
  };
  BML_Expr *expr = bml_add(ctx, bml_number(ctx, 1.0), bml_number(ctx, 2.0));
  char preorder[4] = "";
  char postorder[4] = "";

  CHECK(bml_validate_ast(expr) == BML_VALID);
  CHECK(bml_validate_ast(&invalid) == BML_INVALID_CHILD_COUNT);
  invalid = (BML_Expr){.type = BML_NODE_NEGATE, .child_count = 1};
  CHECK(bml_validate_ast(&invalid) == BML_INVALID_NULL_NODE);
  CHECK(bml_number(NULL, 1.0) == NULL);
  CHECK(bml_add(ctx, NULL, bml_number(ctx, 1.0)) == NULL);
  CHECK(bml_visit(expr, BML_TRAVERSE_PRE_ORDER, record_visit, preorder) == 0);
  CHECK(bml_visit(expr, BML_TRAVERSE_POST_ORDER, record_visit, postorder) == 0);
  CHECK(strcmp(preorder, "400") == 0);
  CHECK(strcmp(postorder, "004") == 0);
}

static void test_optional_public_apis(BML_Context *ctx) {
  BML_Error error;
  BML_Expr *expr = bml_parse(ctx, "q/q", &error);
  BML_OptimizerResult optimization;
  BML_VerificationResult verification;

  CHECK(expr && error == BML_SUCCESS);
  CHECK(bml_expr_definedness(ctx, expr) == BML_DEFINED_UNKNOWN);
  CHECK(bml_context_assume(ctx, "q", BML_ASSUME_NONZERO | BML_ASSUME_FINITE));
  CHECK(bml_expr_is_defined(ctx, expr));
  CHECK(bml_verify_equivalent(ctx, expr, bml_simplify(ctx, expr), NULL, &verification));
  CHECK(verification.passed > 0);
  CHECK(bml_optimize_with_engine(ctx, bml_parse(ctx, "(x+y)^2-2*x*y", &error),
                                 BML_OPTIMIZE_OPERATION_COUNT, BML_OPTIMIZER_EGRAPH,
                                 &optimization));
  CHECK(optimization.engine == BML_OPTIMIZER_EGRAPH);
}

int main(void) {
  BML_Context *ctx = bml_context_create();
  CHECK(ctx != NULL);
  CHECK(strcmp(bml_error_string(BML_ERROR_PARSE), "parse error") == 0);

  if (ctx) {
    test_arena_growth(ctx);
    test_simplification(ctx);
    test_compiler_core(ctx);
    test_validation_and_visiting(ctx);
    test_optional_public_apis(ctx);
    bml_context_destroy(ctx);
  }

  return failures != 0;
}
