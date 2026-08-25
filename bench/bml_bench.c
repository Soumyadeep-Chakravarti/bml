#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <bml/bml.h>

typedef struct {
  const char *group;
  const char *input;
} Benchmark;
static const Benchmark corpus[] = {
    {"algebra", "2*x + x + 0"},          {"calculus", "sin(x)^2 + cos(x)^2 + exp(x)"},
    {"cse", "(x + y)^2 + sin(x + y)"},   {"powers", "(x*y)^4"},
    {"trig", "sin(x + y) + cos(x + y)"}, {"generated", "(a + b + c)^3 + (a + b + c)^2"},
};

static void timestamp(char *buffer, size_t size) {
  time_t now = time(NULL);
  struct tm *utc = gmtime(&now);
  struct tm value = utc ? *utc : (struct tm){0};
  strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", &value);
}

int main(int argc, char **argv) {
  const char *csv_path = NULL, *markdown_path = NULL;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--csv") && i + 1 < argc)
      csv_path = argv[++i];
    else if (!strcmp(argv[i], "--markdown") && i + 1 < argc)
      markdown_path = argv[++i];
    else {
      fprintf(stderr, "usage: bml_bench [--csv FILE] [--markdown FILE]\n");
      return 2;
    }
  }
  FILE *csv = csv_path ? fopen(csv_path, "w") : stdout;
  if (!csv) {
    perror(csv_path);
    return 1;
  }
  BML_Context *ctx = bml_context_create();
  if (!ctx) {
    if (csv_path)
      fclose(csv);
    return 1;
  }
  fprintf(csv, "group,input-nodes,input-cost,optimized-nodes,optimized-cost,time-ms,"
               "egraph-generated\n");
  size_t total_input = 0, total_optimized = 0;
  double total_ms = 0.0;
  for (size_t i = 0; i < sizeof(corpus) / sizeof(*corpus); i++) {
    BML_Error error;
    BML_Expr *expr = bml_parse(ctx, corpus[i].input, &error);
    BML_OptimizerResult result;
    BML_OperationCost input;
    clock_t started = clock();
    if (!expr || !bml_optimize(ctx, expr, BML_OPTIMIZE_C_RUNTIME, &result)) {
      bml_context_destroy(ctx);
      if (csv_path)
        fclose(csv);
      return 1;
    }
    double milliseconds = 1000.0 * (double)(clock() - started) / CLOCKS_PER_SEC;
    bml_analyze_cost(expr, &input);
    total_input += input.estimated_c_operations;
    total_optimized +=
        result.candidates[result.selected_candidate].cost.estimated_c_operations;
    total_ms += milliseconds;
    fprintf(csv, "%s,%zu,%zu,%zu,%zu,%.3f,%zu\n", corpus[i].group, input.nodes,
            input.estimated_c_operations,
            result.candidates[result.selected_candidate].cost.nodes,
            result.candidates[result.selected_candidate].cost.estimated_c_operations,
            milliseconds, result.egraph.generated);
  }
  if (csv_path)
    fclose(csv);
  if (markdown_path) {
    FILE *report = fopen(markdown_path, "w");
    char generated[32];
    timestamp(generated, sizeof(generated));
    if (!report) {
      perror(markdown_path);
      bml_context_destroy(ctx);
      return 1;
    }
    fprintf(report,
            "# BML benchmark report\n\n- Version: %s\n- Generated: %s\n- Command: "
            "`bml_bench --csv results.csv --markdown report.md`\n\n",
            BML_VERSION_STRING, generated);
    fprintf(report,
            "| Benchmarks | Input C operations | Optimized C operations | Total CPU ms "
            "|\n|---:|---:|---:|---:|\n| %zu | %zu | %zu | %.3f |\n",
            sizeof(corpus) / sizeof(*corpus), total_input, total_optimized, total_ms);
    fclose(report);
  }
  bml_context_destroy(ctx);
  return 0;
}
