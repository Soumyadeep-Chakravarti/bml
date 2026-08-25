#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <bml/bml.h>

static void print_expr(const BML_Expr *expr, int c_mode) {
  char output[4096];
  BML_Error error = c_mode ? bml_emit_c(expr, output, sizeof(output))
                           : bml_format_infix(expr, output, sizeof(output));
  if (error == BML_SUCCESS)
    puts(output);
}

static void trace(const char *rule, const BML_Expr *before, const BML_Expr *after,
                  void *userdata) {
  (void)userdata;
  char a[1024], b[1024];
  bml_format_infix(before, a, sizeof(a));
  bml_format_infix(after, b, sizeof(b));
  printf("%s: %s -> %s\n", rule, a, b);
}

static void collect_variables(const BML_Expr *expr, const char **names, size_t *count);

static int parse_target(const char *text, BML_OptimizationTarget *target) {
  if (!strcmp(text, "c"))
    *target = BML_OPTIMIZE_C_RUNTIME;
  else if (!strcmp(text, "operations"))
    *target = BML_OPTIMIZE_OPERATION_COUNT;
  else if (!strcmp(text, "readability"))
    *target = BML_OPTIMIZE_READABILITY;
  else
    return 0;
  return 1;
}

static int parse_engine(const char *text, BML_OptimizerEngine *engine) {
  if (!strcmp(text, "local"))
    *engine = BML_OPTIMIZER_LOCAL;
  else if (!strcmp(text, "egraph"))
    *engine = BML_OPTIMIZER_EGRAPH;
  else
    return 0;
  return 1;
}

static void print_optimization(BML_Context *ctx, const BML_Expr *expr,
                               BML_OptimizationTarget target,
                               BML_OptimizerEngine engine, int explain) {
  BML_OptimizerResult result;
  char text[4096];
  if (!bml_optimize_with_engine(ctx, expr, target, engine, &result))
    return;
  if (!explain) {
    print_expr(result.selected, 0);
    return;
  }
  bml_format_infix(result.canonical, text, sizeof(text));
  printf("canonical: %s\n", text);
  printf("detected CSE: %zu\n", result.cse_count);
  for (size_t i = 0; i < result.candidate_count; i++) {
    BML_OptimizationCandidate *candidate = &result.candidates[i];
    bml_format_infix(candidate->expression, text, sizeof(text));
    printf("candidate %s: nodes=%zu mathematical-operations=%zu c-operations=%zu "
           "score=%zu: %s\n",
           candidate->form, candidate->cost.nodes,
           candidate->cost.mathematical_operations,
           candidate->cost.estimated_c_operations, candidate->score, text);
  }
  printf("selected: %s\n", result.candidates[result.selected_candidate].form);
  if (result.engine == BML_OPTIMIZER_EGRAPH) {
    printf("experimental egraph: iterations=%zu generated=%zu node-limit=%zu\n",
           result.egraph.iterations, result.egraph.generated, result.egraph.node_limit);
    if (result.proof_steps)
      printf("proof: %s\n", result.proof_steps);
  }
  if (target == BML_OPTIMIZE_C_RUNTIME) {
    const char *parameters[64];
    size_t parameter_count = 0;
    collect_variables(expr, parameters, &parameter_count);
    if (bml_emit_c_optimized(expr, "bml_expression", parameters, parameter_count, text,
                             sizeof(text)) == BML_SUCCESS)
      printf("generated C:\n%s", text);
  }
}

static int add_assumption(BML_Context *ctx, const char *text) {
  const char *colon = strchr(text, ':');
  uint32_t flags = BML_ASSUME_NONE;
  if (!colon)
    return 0;
  if (!strcmp(colon + 1, "real"))
    flags = BML_ASSUME_REAL;
  else if (!strcmp(colon + 1, "finite"))
    flags = BML_ASSUME_FINITE;
  else if (!strcmp(colon + 1, "positive"))
    flags = BML_ASSUME_POSITIVE;
  else if (!strcmp(colon + 1, "nonnegative"))
    flags = BML_ASSUME_NONNEGATIVE;
  else if (!strcmp(colon + 1, "integer"))
    flags = BML_ASSUME_INTEGER;
  else if (!strcmp(colon + 1, "nonzero"))
    flags = BML_ASSUME_NONZERO;
  else
    return 0;
  size_t length = (size_t)(colon - text);
  if (!length || length >= 128)
    return 0;
  char name[128];
  memcpy(name, text, length);
  name[length] = '\0';
  return bml_context_assume(ctx, name, flags);
}

static void print_verification(const BML_VerificationResult *result) {
  printf("trials=%zu passed=%zu max-absolute-error=%.17g", result->trials,
         result->passed, result->max_absolute_error);
  if (result->first_failure != SIZE_MAX)
    printf(" first-failure=%zu", result->first_failure);
  putchar('\n');
}

static void print_benchmark(BML_Context *ctx, const BML_Expr *expr,
                            BML_OptimizationTarget target, BML_OptimizerEngine engine) {
  BML_OptimizerResult result;
  BML_OperationCost input;
  clock_t started = clock();
  if (!bml_optimize_with_engine(ctx, expr, target, engine, &result))
    return;
  double milliseconds = 1000.0 * (double)(clock() - started) / CLOCKS_PER_SEC;
  bml_analyze_cost(expr, &input);
  printf("input nodes=%zu cost=%zu optimized nodes=%zu cost=%zu time-ms=%.3f\n",
         input.nodes, input.estimated_c_operations,
         result.candidates[result.selected_candidate].cost.nodes,
         result.candidates[result.selected_candidate].cost.estimated_c_operations,
         milliseconds);
  if (engine == BML_OPTIMIZER_EGRAPH)
    printf("egraph iterations=%zu generated=%zu node-limit=%zu\n",
           result.egraph.iterations, result.egraph.generated, result.egraph.node_limit);
}

static void collect_variables(const BML_Expr *expr, const char **names, size_t *count) {
  if (expr->type == BML_NODE_VARIABLE) {
    for (size_t i = 0; i < *count; i++)
      if (!strcmp(names[i], expr->value.identifier))
        return;
    if (*count < 64)
      names[(*count)++] = expr->value.identifier;
  }
  for (size_t i = 0; i < expr->child_count; i++)
    collect_variables(expr->children[i], names, count);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: bml "
                    "<parse|simplify|eval|diff|emit-c|explain|optimize|verify|verify-"
                    "diff|benchmark> EXPR [args]\n");
    return 2;
  }
  BML_Context *ctx = bml_context_create();
  int expression_index = 2;
  int optimize_c = 0;
  BML_OptimizationTarget target = BML_OPTIMIZE_C_RUNTIME;
  BML_OptimizerEngine engine = BML_OPTIMIZER_LOCAL;
  int optimize_explain = 0;
  if (!strcmp(argv[1], "emit-c") && argc > 3 && !strcmp(argv[2], "--optimize")) {
    optimize_c = 1;
    expression_index = 3;
  }
  if (!strcmp(argv[1], "emit-c") && argc > 3 && !strcmp(argv[3], "--optimize"))
    optimize_c = 1;
  BML_Error error;
  BML_Expr *expr = bml_parse(ctx, argv[expression_index], &error);
  if (!expr) {
    fprintf(stderr, "parse error\n");
    bml_context_destroy(ctx);
    return 1;
  }
  for (int i = expression_index + 1; i + 1 < argc; i++)
    if (!strcmp(argv[i], "--assume") && !add_assumption(ctx, argv[++i]))
      error = BML_ERROR_INVALID_ARGUMENT;
  for (int i = expression_index + 1; i < argc; i++) {
    if (!strcmp(argv[i], "--explain"))
      optimize_explain = 1;
    else if (!strncmp(argv[i], "--target=", 9))
      parse_target(argv[i] + 9, &target);
    else if (!strcmp(argv[i], "--target") && i + 1 < argc)
      parse_target(argv[++i], &target);
    else if (!strncmp(argv[i], "--engine=", 9))
      parse_engine(argv[i] + 9, &engine);
    else if (!strcmp(argv[i], "--engine") && i + 1 < argc)
      parse_engine(argv[++i], &engine);
  }
  if (!strcmp(argv[1], "parse"))
    print_expr(expr, 0);
  else if (!strcmp(argv[1], "simplify"))
    print_expr(bml_simplify(ctx, expr), 0);
  else if (!strcmp(argv[1], "emit-c")) {
    if (!optimize_c)
      print_expr(expr, 1);
    else {
      const char *parameters[64];
      size_t parameter_count = 0;
      char output[4096];
      collect_variables(expr, parameters, &parameter_count);
      error = bml_emit_c_optimized(expr, "bml_expression", parameters, parameter_count,
                                   output, sizeof(output));
      if (error == BML_SUCCESS)
        puts(output);
    }
  } else if (!strcmp(argv[1], "optimize")) {
    print_optimization(ctx, expr, target, engine, optimize_explain);
  } else if (!strcmp(argv[1], "benchmark")) {
    print_benchmark(ctx, expr, target, engine);
  } else if (!strcmp(argv[1], "verify") && argc >= 4) {
    BML_Expr *candidate = bml_parse(ctx, argv[3], &error);
    BML_VerificationResult result;
    if (!candidate || !bml_verify_equivalent(ctx, expr, candidate, NULL, &result)) {
      print_verification(&result);
      error = result.error == BML_SUCCESS ? BML_ERROR_DOMAIN : result.error;
    } else
      print_verification(&result);
  } else if (!strcmp(argv[1], "verify-diff") && argc >= 4) {
    BML_Expr *derivative = bml_differentiate(ctx, expr, argv[3]);
    BML_VerificationResult result;
    if (!derivative ||
        !bml_verify_derivative(ctx, expr, derivative, argv[3], NULL, &result)) {
      print_verification(&result);
      error = result.error == BML_SUCCESS ? BML_ERROR_DOMAIN : result.error;
    } else
      print_verification(&result);
  } else if (!strcmp(argv[1], "explain"))
    print_expr(bml_simplify_traced(ctx, expr, trace, NULL), 0);
  else if (!strcmp(argv[1], "diff") && argc == 4)
    print_expr(bml_simplify(ctx, bml_differentiate(ctx, expr, argv[3])), 0);
  else if (!strcmp(argv[1], "eval") && argc >= 4) {
    BML_Binding bindings[64];
    size_t count = 0;
    for (int i = expression_index + 1; i < argc && count < 64; i++) {
      if (!strcmp(argv[i], "--assume")) {
        i++;
        continue;
      }
      char *equal = strchr(argv[i], '=');
      if (!equal) {
        error = BML_ERROR_INVALID_ARGUMENT;
        break;
      }
      *equal = '\0';
      bindings[count++] = (BML_Binding){argv[i], strtod(equal + 1, NULL)};
      *equal = '=';
    }
    double value;
    error = error == BML_SUCCESS ? bml_evaluate(expr, bindings, count, &value) : error;
    if (error == BML_SUCCESS)
      printf("%.17g\n", value);
    else
      fprintf(stderr, "evaluation error\n");
  } else {
    fprintf(stderr, "invalid command arguments\n");
    error = BML_ERROR_INVALID_ARGUMENT;
  }
  bml_context_destroy(ctx);
  return error == BML_SUCCESS ? 0 : 1;
}
