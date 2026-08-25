#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <bml/bml.h>

static void usage(FILE *stream) {
  fprintf(stream, "usage: bml "
                  "<parse|simplify|eval|diff|emit-c|explain|optimize|verify|verify-"
                  "diff|benchmark> EXPR [args] [options]\n"
                  "options: --assume NAME:FACT --target={c,operations,readability} "
                  "--engine={local,egraph} --explain\n");
}

static void print_expr(const BML_Expr *expr, int c_mode) {
  char output[4096];
  BML_Error error = c_mode ? bml_emit_c(expr, output, sizeof(output))
                           : bml_format_infix(expr, output, sizeof(output));
  if (error == BML_SUCCESS)
    puts(output);
}

static void print_parse_error(const char *source, BML_Error error,
                              const BML_ParseDiagnostic *diagnostic) {
  fprintf(stderr, "%s at offset %zu: %s\n%s\n%*s^\n", bml_error_string(error),
          diagnostic->offset, diagnostic->message, source, (int)diagnostic->offset, "");
}

static void trace(const char *rule, const BML_Expr *before, const BML_Expr *after,
                  void *userdata) {
  (void)userdata;
  char first[1024], second[1024];
  bml_format_infix(before, first, sizeof(first));
  bml_format_infix(after, second, sizeof(second));
  printf("%s: %s -> %s\n", rule, first, second);
}

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

static int add_assumption(BML_Context *ctx, const char *text) {
  const char *colon = strchr(text, ':');
  uint32_t flags = BML_ASSUME_NONE;
  if (!colon || colon == text)
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
  if (length >= 128)
    return 0;
  char name[128];
  memcpy(name, text, length);
  name[length] = '\0';
  return bml_context_assume(ctx, name, flags);
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
  printf("canonical: %s\ndetected CSE: %zu\n", text, result.cse_count);
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
}

int main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--version")) {
    puts(BML_VERSION_STRING);
    return 0;
  }
  if (argc < 3) {
    usage(stderr);
    return 2;
  }
  const char *command = argv[1], *source = argv[2], *argument = NULL;
  int index = 3, optimize_c = 0, explain = 0;
  if (!strcmp(command, "emit-c") && !strcmp(source, "--optimize")) {
    if (argc < 4) {
      usage(stderr);
      return 2;
    }
    optimize_c = 1;
    source = argv[3];
    index = 4;
  }
  BML_OptimizationTarget target = BML_OPTIMIZE_C_RUNTIME;
  BML_OptimizerEngine engine = BML_OPTIMIZER_LOCAL;
  if (!strcmp(command, "emit-c") && index < argc &&
      !strcmp(argv[index], "--optimize")) {
    optimize_c = 1;
    index++;
  }
  if ((!strcmp(command, "diff") || !strcmp(command, "verify") ||
       !strcmp(command, "verify-diff")) &&
      index < argc && argv[index][0] != '-')
    argument = argv[index++];
  BML_Context *ctx = bml_context_create();
  if (!ctx) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }
  for (; index < argc; index++) {
    const char *value = argv[index];
    if (!strcmp(command, "eval") && strchr(value, '='))
      continue;
    if (!strcmp(value, "--optimize") && !strcmp(command, "emit-c"))
      optimize_c = 1;
    else if (!strcmp(value, "--explain"))
      explain = 1;
    else if (!strcmp(value, "--assume") && index + 1 < argc &&
             add_assumption(ctx, argv[++index]))
      continue;
    else if (!strncmp(value, "--target=", 9) && parse_target(value + 9, &target))
      continue;
    else if (!strcmp(value, "--target") && index + 1 < argc &&
             parse_target(argv[++index], &target))
      continue;
    else if (!strncmp(value, "--engine=", 9) && parse_engine(value + 9, &engine))
      continue;
    else if (!strcmp(value, "--engine") && index + 1 < argc &&
             parse_engine(argv[++index], &engine))
      continue;
    else {
      fprintf(stderr, "%s: %s\n",
              !strncmp(value, "--target", 8)   ? "invalid optimization target"
              : !strncmp(value, "--engine", 8) ? "invalid optimizer engine"
                                               : "invalid option or argument",
              value);
      usage(stderr);
      bml_context_destroy(ctx);
      return 2;
    }
  }
  BML_Error error;
  BML_ParseDiagnostic diagnostic;
  BML_Expr *expr = bml_parse_with_diagnostic(ctx, source, &error, &diagnostic);
  if (!expr) {
    print_parse_error(source, error, &diagnostic);
    bml_context_destroy(ctx);
    return 1;
  }
  if (!strcmp(command, "parse"))
    print_expr(expr, 0);
  else if (!strcmp(command, "simplify"))
    print_expr(bml_simplify(ctx, expr), 0);
  else if (!strcmp(command, "explain"))
    print_expr(bml_simplify_traced(ctx, expr, trace, NULL), 0);
  else if (!strcmp(command, "emit-c")) {
    if (!optimize_c)
      print_expr(expr, 1);
    else {
      const char *names[64];
      size_t count = 0;
      char output[4096];
      collect_variables(expr, names, &count);
      error = bml_emit_c_optimized(expr, "bml_expression", names, count, output,
                                   sizeof(output));
      if (error == BML_SUCCESS)
        puts(output);
    }
  } else if (!strcmp(command, "optimize"))
    print_optimization(ctx, expr, target, engine, explain);
  else if (!strcmp(command, "diff") && argument)
    print_expr(bml_simplify(ctx, bml_differentiate(ctx, expr, argument)), 0);
  else if (!strcmp(command, "verify") && argument) {
    BML_ParseDiagnostic candidate_diagnostic;
    BML_Expr *candidate =
        bml_parse_with_diagnostic(ctx, argument, &error, &candidate_diagnostic);
    BML_VerificationResult result;
    if (!candidate)
      print_parse_error(argument, error, &candidate_diagnostic);
    else if (bml_verify_equivalent(ctx, expr, candidate, NULL, &result))
      printf("trials=%zu passed=%zu max-absolute-error=%.17g\n", result.trials,
             result.passed, result.max_absolute_error);
    else {
      printf("trials=%zu passed=%zu max-absolute-error=%.17g\n", result.trials,
             result.passed, result.max_absolute_error);
      error = result.error == BML_SUCCESS ? BML_ERROR_DOMAIN : result.error;
    }
  } else if (!strcmp(command, "verify-diff") && argument) {
    BML_VerificationResult result;
    BML_Expr *derivative = bml_differentiate(ctx, expr, argument);
    int ok = derivative &&
             bml_verify_derivative(ctx, expr, derivative, argument, NULL, &result);
    printf("trials=%zu passed=%zu max-absolute-error=%.17g\n", result.trials,
           result.passed, result.max_absolute_error);
    if (!ok)
      error = result.error == BML_SUCCESS ? BML_ERROR_DOMAIN : result.error;
  } else if (!strcmp(command, "eval") && argc > 3) {
    BML_Binding bindings[64];
    size_t count = 0;
    for (int i = 3; i < argc && count < 64; i++) {
      char *equal = strchr(argv[i], '=');
      if (!equal)
        continue;
      *equal = '\0';
      bindings[count++] = (BML_Binding){argv[i], strtod(equal + 1, NULL)};
      *equal = '=';
    }
    double result;
    error = bml_evaluate(expr, bindings, count, &result);
    if (error == BML_SUCCESS)
      printf("%.17g\n", result);
  } else if (!strcmp(command, "benchmark")) {
    BML_OptimizerResult result;
    BML_OperationCost input;
    clock_t started = clock();
    error = bml_optimize_with_engine(ctx, expr, target, engine, &result)
                ? BML_SUCCESS
                : BML_ERROR_INTERNAL;
    bml_analyze_cost(expr, &input);
    printf("input nodes=%zu cost=%zu optimized nodes=%zu cost=%zu time-ms=%.3f\n",
           input.nodes, input.estimated_c_operations,
           result.candidates[result.selected_candidate].cost.nodes,
           result.candidates[result.selected_candidate].cost.estimated_c_operations,
           1000.0 * (double)(clock() - started) / CLOCKS_PER_SEC);
  } else {
    fprintf(stderr, "invalid command arguments for '%s'\n", command);
    usage(stderr);
    error = BML_ERROR_INVALID_ARGUMENT;
  }
  if (error != BML_SUCCESS)
    fprintf(stderr, "%s\n", bml_error_string(error));
  bml_context_destroy(ctx);
  return error == BML_SUCCESS ? 0 : 1;
}
