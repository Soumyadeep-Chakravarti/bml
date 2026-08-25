#ifndef BML_SIMPLIFY_H
#define BML_SIMPLIFY_H

#include "ast.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BML_ASSUME_NONE = 0,
  BML_ASSUME_NONZERO = 1u << 0,
  BML_ASSUME_REAL = 1u << 1,
  BML_ASSUME_POSITIVE = 1u << 2,
  BML_ASSUME_INTEGER = 1u << 3,
  BML_ASSUME_FINITE = 1u << 4,
  BML_ASSUME_NONNEGATIVE = 1u << 5,
} BML_AssumptionFlags;

typedef struct BML_SymbolAssumption {
  const char *name;
  uint32_t flags;
} BML_SymbolAssumption;

/* Associates flags with a named symbol for transformations in this context. */
int bml_context_assume(BML_Context *ctx, const char *name, uint32_t flags);
uint32_t bml_context_assumptions(const BML_Context *ctx, const char *name);
typedef struct {
  size_t max_iterations; /* Zero selects the default (16). */
  size_t rewrite_budget; /* Zero selects the default (256). */
} BML_SimplifyOptions;

typedef enum {
  BML_RULE_CANONICAL_ASSOCIATIVE,
  BML_RULE_DIVIDE_SELF,
  BML_RULE_POWER_ZERO,
  BML_RULE_POWER_ONE,
  BML_RULE_COUNT
} BML_Rule;

typedef struct {
  const char *id;
  const char *category;
  unsigned priority;
  uint32_t required_assumptions;
  const char *requirement;
} BML_RuleMetadata;

/* Metadata is static for the lifetime of the library. */
const BML_RuleMetadata *bml_rule_metadata(BML_Rule rule);

typedef struct {
  const BML_RuleMetadata *rule;
  const char *id;          /* Stable rule ID, for example "bml.add.combine". */
  const char *requirement; /* NULL when the rule is unconditional. */
  const BML_Expr *before;
  const BML_Expr *after;
} BML_RewriteEvent;

typedef void (*BML_TraceCallback)(const BML_RewriteEvent *event, void *userdata);

BML_Expr *bml_canonicalize(BML_Context *ctx, const BML_Expr *expr);
BML_Expr *bml_simplify_with_options(BML_Context *ctx, const BML_Expr *expr,
                                    const BML_SimplifyOptions *options,
                                    BML_TraceCallback callback, void *userdata);
BML_Expr *bml_simplify(BML_Context *ctx, const BML_Expr *expr);

typedef void (*BML_RewriteCallback)(const char *rule, const BML_Expr *before,
                                    const BML_Expr *after, void *userdata);

/* Invokes callback once for each successful local rewrite, in traversal order. */
BML_Expr *bml_simplify_traced(BML_Context *ctx, const BML_Expr *expr,
                              BML_RewriteCallback callback, void *userdata);

/* Deterministic structural identity and target-specific cost models. */
uint64_t bml_expr_hash(const BML_Expr *expr);
typedef struct {
  size_t nodes;
  size_t adds;
  size_t subtracts;
  size_t multiplies;
  size_t divides;
  size_t powers;
  size_t calls;
  /* Each scalar AST operation, including power and function calls, counts once. */
  size_t mathematical_operations;
  /* Lowering estimate: powers and calls are weighted more heavily than arithmetic. */
  size_t estimated_c_operations;
} BML_OperationCost;
void bml_analyze_cost(const BML_Expr *expr, BML_OperationCost *cost);

typedef enum {
  BML_OPTIMIZE_READABILITY,
  BML_OPTIMIZE_OPERATION_COUNT,
  BML_OPTIMIZE_C_RUNTIME,
} BML_OptimizationTarget;

typedef enum { BML_OPTIMIZER_LOCAL, BML_OPTIMIZER_EGRAPH } BML_OptimizerEngine;

typedef struct {
  size_t iterations;
  size_t generated;
  size_t node_limit;
} BML_EGraphMetrics;

typedef struct {
  const char *form;
  const BML_Expr *expression;
  BML_OperationCost cost;
  size_t score; /* Nodes, mathematical operations, or C-runtime estimate by target. */
} BML_OptimizationCandidate;

typedef struct {
  const BML_Expr *canonical;
  const BML_Expr *selected;
  BML_OptimizationTarget target;
  BML_OptimizationCandidate candidates[3];
  size_t candidate_count;
  size_t selected_candidate;
  size_t cse_count;
  BML_OptimizerEngine engine;
  BML_EGraphMetrics egraph;
  const char *proof_steps; /* NULL for local optimization. */
} BML_OptimizerResult;

/* Result expression pointers are owned by ctx and remain valid until it is destroyed.
 */
int bml_optimize(BML_Context *ctx, const BML_Expr *expr, BML_OptimizationTarget target,
                 BML_OptimizerResult *result);
int bml_optimize_with_engine(BML_Context *ctx, const BML_Expr *expr,
                             BML_OptimizationTarget target, BML_OptimizerEngine engine,
                             BML_OptimizerResult *result);

#ifdef __cplusplus
}
#endif

#endif /* BML_SIMPLIFY_H */
