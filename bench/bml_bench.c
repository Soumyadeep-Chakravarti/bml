#include <stdio.h>
#include <time.h>

#include <bml/bml.h>

typedef struct {
  const char *group;
  const char *input;
} Benchmark;

/* The generated group is deterministic input generation represented as infix. */
static const Benchmark corpus[] = {
    {"algebra", "2*x + x + 0"},          {"calculus", "sin(x)^2 + cos(x)^2 + exp(x)"},
    {"cse", "(x + y)^2 + sin(x + y)"},   {"powers", "(x*y)^4"},
    {"trig", "sin(x + y) + cos(x + y)"}, {"generated", "(a + b + c)^3 + (a + b + c)^2"},
};

int main(void) {
  BML_Context *ctx = bml_context_create();
  if (!ctx)
    return 1;
  puts("group,input-nodes,input-cost,optimized-nodes,optimized-cost,time-ms,egraph-"
       "generated");
  for (size_t i = 0; i < sizeof(corpus) / sizeof(*corpus); i++) {
    BML_Error error;
    BML_Expr *expr = bml_parse(ctx, corpus[i].input, &error);
    BML_OptimizerResult result;
    BML_OperationCost input;
    clock_t started = clock();
    if (!expr || !bml_optimize(ctx, expr, BML_OPTIMIZE_C_RUNTIME, &result)) {
      bml_context_destroy(ctx);
      return 1;
    }
    double milliseconds = 1000.0 * (double)(clock() - started) / CLOCKS_PER_SEC;
    bml_analyze_cost(expr, &input);
    printf("%s,%zu,%zu,%zu,%zu,%.3f,%zu\n", corpus[i].group, input.nodes,
           input.estimated_c_operations,
           result.candidates[result.selected_candidate].cost.nodes,
           result.candidates[result.selected_candidate].cost.estimated_c_operations,
           milliseconds, result.egraph.generated);
  }
  bml_context_destroy(ctx);
  return 0;
}
