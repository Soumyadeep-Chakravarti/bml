#include "bml/core/simplify.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bml/core/arena.h"
#include "bml/core/ast_builder.h"
#include "bml/core/context.h"
#include "bml/internal/egraph.h"
#include "bml/internal/expr_utils.h"
#include "bml/semantics.h"

struct BML_ExprMemoEntry {
  uint64_t hash;
  BML_Expr *expression;
};

static const BML_RuleMetadata rules[BML_RULE_COUNT] = {
    [BML_RULE_CANONICAL_ASSOCIATIVE] = {"bml.canonical.associative", "canonicalize", 10,
                                        BML_ASSUME_NONE, NULL},
    [BML_RULE_DIVIDE_SELF] = {"bml.divide.self", "simplify", 20, BML_ASSUME_NONZERO,
                              "nonzero"},
    [BML_RULE_POWER_ZERO] = {"bml.power.zero", "simplify", 20, BML_ASSUME_NONZERO,
                             "nonzero"},
    [BML_RULE_POWER_ONE] = {"bml.power.one", "simplify", 30, BML_ASSUME_NONE, NULL},
};

const BML_RuleMetadata *bml_rule_metadata(BML_Rule rule) {
  return rule < BML_RULE_COUNT ? &rules[rule] : NULL;
}

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size) {
  const unsigned char *bytes = data;
  for (size_t i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

uint64_t bml_expr_hash(const BML_Expr *expr) {
  uint64_t hash = UINT64_C(1469598103934665603);
  if (!expr)
    return hash_bytes(hash, "null", 4);
  hash = hash_bytes(hash, &expr->type, sizeof(expr->type));
  if (expr->type == BML_NODE_NUMBER)
    hash = hash_bytes(hash, &expr->value.number, sizeof(expr->value.number));
  else if (expr->type == BML_NODE_VARIABLE || expr->type == BML_NODE_CONSTANT ||
           expr->type == BML_NODE_FUNCTION)
    hash = hash_bytes(hash, expr->value.identifier, strlen(expr->value.identifier));
  for (size_t i = 0; i < expr->child_count; i++) {
    uint64_t child = bml_expr_hash(expr->children[i]);
    hash = hash_bytes(hash, &child, sizeof(child));
  }
  return hash;
}

static int compare_expr(const void *left, const void *right) {
  const BML_Expr *a = *(const BML_Expr *const *)left;
  const BML_Expr *b = *(const BML_Expr *const *)right;
  int arank = a->type == BML_NODE_MULTIPLY ? 3
              : a->type == BML_NODE_NEGATE ? 4
                                           : a->type;
  int brank = b->type == BML_NODE_MULTIPLY ? 3
              : b->type == BML_NODE_NEGATE ? 4
                                           : b->type;
  if (arank != brank)
    return arank < brank ? -1 : 1;
  if (a->type == BML_NODE_VARIABLE || a->type == BML_NODE_CONSTANT ||
      a->type == BML_NODE_FUNCTION)
    return strcmp(a->value.identifier, b->value.identifier);
  if (a->type == BML_NODE_NUMBER)
    return a->value.number < b->value.number ? -1 : a->value.number > b->value.number;
  uint64_t ah = bml_expr_hash(a), bh = bml_expr_hash(b);
  if (ah != bh)
    return ah < bh ? -1 : 1;
  if (bml_expr_equal(a, b))
    return 0;
  return a < b ? -1 : 1; /* Hash collisions remain deterministic within a process. */
}

static int append(BML_Expr ***items, size_t *count, size_t *capacity, BML_Expr *item) {
  if (*count == *capacity) {
    size_t next = *capacity ? *capacity * 2 : 8;
    BML_Expr **grown = realloc(*items, next * sizeof(**items));
    if (!grown)
      return 0;
    *items = grown;
    *capacity = next;
  }
  (*items)[(*count)++] = item;
  return 1;
}

static BML_Expr *canonical(BML_Context *ctx, const BML_Expr *expr);

static int collect(BML_Context *ctx, const BML_Expr *expr, BML_NodeType type,
                   BML_Expr ***items, size_t *count, size_t *capacity) {
  if (expr->type == type) {
    return collect(ctx, expr->children[0], type, items, count, capacity) &&
           collect(ctx, expr->children[1], type, items, count, capacity);
  }
  BML_Expr *item = canonical(ctx, expr);
  return item && append(items, count, capacity, item);
}

static BML_Expr *rebuild(BML_Context *ctx, BML_NodeType type, BML_Expr **items,
                         size_t count) {
  if (!count)
    return bml_number(ctx, type == BML_NODE_ADD ? 0.0 : 1.0);
  BML_Expr *result = items[0];
  for (size_t i = 1; i < count; i++)
    result = type == BML_NODE_ADD ? bml_add(ctx, result, items[i])
                                  : bml_multiply(ctx, result, items[i]);
  return result;
}

static int split_coefficient(const BML_Expr *expr, double *coefficient,
                             const BML_Expr **base) {
  if (expr->type == BML_NODE_MULTIPLY && bml_is_number(expr->children[0])) {
    *coefficient = expr->children[0]->value.number;
    *base = expr->children[1];
    return 1;
  }
  *coefficient = 1.0;
  *base = expr;
  return 1;
}

static BML_Expr *canonical_associative(BML_Context *ctx, const BML_Expr *expr) {
  BML_Expr **items = NULL;
  size_t count = 0, capacity = 0, kept = 0;
  double number = expr->type == BML_NODE_ADD ? 0.0 : 1.0;
  int has_number = 0;
  if (!collect(ctx, expr, expr->type, &items, &count, &capacity)) {
    free(items);
    return NULL;
  }
  for (size_t i = 0; i < count; i++) {
    if (bml_is_number(items[i])) {
      has_number = 1;
      number = expr->type == BML_NODE_ADD ? number + items[i]->value.number
                                          : number * items[i]->value.number;
    } else
      items[kept++] = items[i];
  }
  if (expr->type == BML_NODE_MULTIPLY && has_number && number == 0.0 &&
      bml_expr_is_defined(ctx, expr)) {
    free(items);
    return bml_number(ctx, 0.0);
  }
  qsort(items, kept, sizeof(*items), compare_expr);
  size_t output = 0;
  if (expr->type == BML_NODE_ADD) {
    unsigned char *used = kept ? calloc(kept, sizeof(*used)) : NULL;
    if (kept && !used) {
      free(items);
      return NULL;
    }
    for (size_t i = 0; i < kept; i++) {
      if (used[i])
        continue;
      double coefficient;
      const BML_Expr *base;
      split_coefficient(items[i], &coefficient, &base);
      for (size_t j = i + 1; j < kept; j++) {
        double part;
        const BML_Expr *part_base;
        split_coefficient(items[j], &part, &part_base);
        if (!used[j] && bml_expr_equal(base, part_base)) {
          coefficient += part;
          used[j] = 1;
        }
      }
      if (coefficient == 1.0)
        items[output++] = (BML_Expr *)base;
      else if (coefficient != 0.0)
        items[output++] =
            bml_multiply(ctx, bml_number(ctx, coefficient), (BML_Expr *)base);
    }
    free(used);
  } else
    for (size_t i = 0; i < kept;) {
      size_t repeats = 1;
      while (i + repeats < kept && bml_expr_equal(items[i], items[i + repeats]))
        repeats++;
      if (expr->type == BML_NODE_MULTIPLY && repeats > 1)
        items[output++] = bml_power(ctx, items[i], bml_number(ctx, (double)repeats));
      else
        items[output++] = items[i];
      i += repeats;
    }
  if (has_number && (expr->type == BML_NODE_ADD ? number != 0.0 : number != 1.0)) {
    memmove(items + 1, items, output * sizeof(*items));
    items[0] = bml_number(ctx, number);
    output++;
  }
  BML_Expr *result = rebuild(ctx, expr->type, items, output);
  free(items);
  return result;
}

static BML_Expr *canonical(BML_Context *ctx, const BML_Expr *expr) {
  if (!expr)
    return NULL;
  uint64_t hash = bml_expr_hash(expr);
  for (size_t i = 0; i < ctx->canonical_memo_count; i++)
    if (ctx->canonical_memo[i].hash == hash)
      return ctx->canonical_memo[i].expression;
  BML_Expr *result;
  if (expr->type == BML_NODE_ADD || expr->type == BML_NODE_MULTIPLY)
    result = canonical_associative(ctx, expr);
  else if (!expr->child_count)
    result = expr->type == BML_NODE_NUMBER ? bml_number(ctx, expr->value.number)
             : expr->type == BML_NODE_CONSTANT
                 ? bml_constant(ctx, expr->value.identifier)
                 : bml_variable(ctx, expr->value.identifier);
  else {
    BML_Expr *left = canonical(ctx, expr->children[0]);
    if (!left)
      return NULL;
    if (expr->type == BML_NODE_NEGATE)
      result = bml_negate(ctx, left);
    else if (expr->type == BML_NODE_FUNCTION)
      result = bml_function(ctx, expr->value.identifier, left);
    else {
      BML_Expr *right = canonical(ctx, expr->children[1]);
      if (!right)
        return NULL;
      switch (expr->type) {
      case BML_NODE_SUBTRACT:
        result = bml_subtract(ctx, left, right);
        break;
      case BML_NODE_DIVIDE:
        result = bml_divide(ctx, left, right);
        break;
      case BML_NODE_POWER:
        result = bml_power(ctx, left, right);
        break;
      default:
        result = NULL;
        break;
      }
    }
  }
  if (!result)
    return NULL;
  if (ctx->canonical_memo_count == ctx->canonical_memo_capacity) {
    size_t capacity =
        ctx->canonical_memo_capacity ? ctx->canonical_memo_capacity * 2 : 32;
    struct BML_ExprMemoEntry *entries =
        realloc(ctx->canonical_memo, capacity * sizeof(*entries));
    if (!entries)
      return NULL;
    ctx->canonical_memo = entries;
    ctx->canonical_memo_capacity = capacity;
  }
  ctx->canonical_memo[ctx->canonical_memo_count++] =
      (struct BML_ExprMemoEntry){hash, result};
  return result;
}

BML_Expr *bml_canonicalize(BML_Context *ctx, const BML_Expr *expr) {
  return ctx ? canonical(ctx, expr) : NULL;
}

int bml_context_assume(BML_Context *ctx, const char *name, uint32_t flags) {
  if (!ctx || !name)
    return 0;
  for (size_t i = 0; i < ctx->assumption_count; i++)
    if (!strcmp(ctx->assumptions[i].name, name)) {
      ctx->assumptions[i].flags |= flags;
      return 1;
    }
  if (ctx->assumption_count == ctx->assumption_capacity) {
    size_t capacity = ctx->assumption_capacity ? ctx->assumption_capacity * 2 : 8;
    BML_SymbolAssumption *grown = realloc(ctx->assumptions, capacity * sizeof(*grown));
    if (!grown)
      return 0;
    ctx->assumptions = grown;
    ctx->assumption_capacity = capacity;
  }
  size_t length = strlen(name) + 1;
  char *stored = bml_arena_alloc(ctx->arena, length);
  if (!stored)
    return 0;
  memcpy(stored, name, length);
  ctx->assumptions[ctx->assumption_count++] = (BML_SymbolAssumption){stored, flags};
  return 1;
}

uint32_t bml_context_assumptions(const BML_Context *ctx, const char *name) {
  if (!ctx || !name)
    return BML_ASSUME_NONE;
  for (size_t i = 0; i < ctx->assumption_count; i++)
    if (!strcmp(ctx->assumptions[i].name, name)) {
      uint32_t flags = ctx->assumptions[i].flags;
      if (flags & BML_ASSUME_POSITIVE)
        flags |= BML_ASSUME_NONZERO | BML_ASSUME_NONNEGATIVE;
      return flags;
    }
  return BML_ASSUME_NONE;
}

static BML_Expr *simplify_once(BML_Context *ctx, const BML_Expr *expr, size_t *budget,
                               BML_TraceCallback callback, void *userdata) {
  BML_Expr *result = canonical(ctx, expr);
  if (!result || !*budget)
    return result;
  const BML_RuleMetadata *rule = NULL;
  if (result->type == BML_NODE_DIVIDE &&
      bml_expr_equal(result->children[0], result->children[1]) &&
      bml_expr_is_defined(ctx, result)) {
    rule = bml_rule_metadata(BML_RULE_DIVIDE_SELF);
    result = bml_number(ctx, 1.0);
  } else if (result->type == BML_NODE_POWER && bml_is_zero(result->children[1]) &&
             bml_expr_is_defined(ctx, result)) {
    rule = bml_rule_metadata(BML_RULE_POWER_ZERO);
    result = bml_number(ctx, 1.0);
  } else if (result->type == BML_NODE_POWER && bml_is_one(result->children[1])) {
    rule = bml_rule_metadata(BML_RULE_POWER_ONE);
    result = result->children[0];
  }
  if (rule) {
    (*budget)--;
    if (callback)
      callback(&(BML_RewriteEvent){rule, rule->id, rule->requirement, expr, result},
               userdata);
  }
  return result;
}

BML_Expr *bml_simplify_with_options(BML_Context *ctx, const BML_Expr *expr,
                                    const BML_SimplifyOptions *options,
                                    BML_TraceCallback callback, void *userdata) {
  size_t iterations = options && options->max_iterations ? options->max_iterations : 16;
  size_t budget = options && options->rewrite_budget ? options->rewrite_budget : 256;
  BML_Expr *current = expr ? (BML_Expr *)expr : NULL;
  for (size_t i = 0; current && i < iterations; i++) {
    BML_Expr *next = simplify_once(ctx, current, &budget, callback, userdata);
    if (bml_expr_equal(current, next))
      return next;
    current = next;
    if (!budget)
      break;
  }
  return current;
}

BML_Expr *bml_simplify(BML_Context *ctx, const BML_Expr *expr) {
  return bml_simplify_with_options(ctx, expr, NULL, NULL, NULL);
}

typedef struct {
  BML_RewriteCallback callback;
  void *userdata;
} LegacyTrace;
static void legacy_trace(const BML_RewriteEvent *event, void *userdata) {
  LegacyTrace *trace = userdata;
  trace->callback(event->id, event->before, event->after, trace->userdata);
}
BML_Expr *bml_simplify_traced(BML_Context *ctx, const BML_Expr *expr,
                              BML_RewriteCallback callback, void *userdata) {
  LegacyTrace trace = {callback, userdata};
  return bml_simplify_with_options(ctx, expr, NULL, callback ? legacy_trace : NULL,
                                   &trace);
}

static void analyze(const BML_Expr *expr, BML_OperationCost *cost) {
  if (!expr)
    return;
  cost->nodes++;
  switch (expr->type) {
  case BML_NODE_ADD:
    cost->adds++;
    cost->mathematical_operations++;
    cost->estimated_c_operations++;
    break;
  case BML_NODE_SUBTRACT:
    cost->subtracts++;
    cost->mathematical_operations++;
    cost->estimated_c_operations++;
    break;
  case BML_NODE_MULTIPLY:
    cost->multiplies++;
    cost->mathematical_operations++;
    cost->estimated_c_operations++;
    break;
  case BML_NODE_DIVIDE:
    cost->divides++;
    cost->mathematical_operations++;
    cost->estimated_c_operations++;
    break;
  case BML_NODE_POWER:
    cost->powers++;
    cost->mathematical_operations++;
    cost->estimated_c_operations += 4;
    break;
  case BML_NODE_FUNCTION:
    cost->calls++;
    cost->mathematical_operations++;
    cost->estimated_c_operations += 4;
    break;
  default:
    break;
  }
  for (size_t i = 0; i < expr->child_count; i++)
    analyze(expr->children[i], cost);
}
void bml_analyze_cost(const BML_Expr *expr, BML_OperationCost *cost) {
  if (!cost)
    return;
  memset(cost, 0, sizeof(*cost));
  analyze(expr, cost);
}

static BML_Expr *expand_small_powers(BML_Context *ctx, const BML_Expr *expr) {
  if (!expr)
    return NULL;
  if (expr->type == BML_NODE_POWER && bml_is_number(expr->children[1]) &&
      expr->children[1]->value.number >= 2.0 &&
      expr->children[1]->value.number <= 4.0 &&
      floor(expr->children[1]->value.number) == expr->children[1]->value.number) {
    unsigned exponent = (unsigned)expr->children[1]->value.number;
    BML_Expr *base = expand_small_powers(ctx, expr->children[0]);
    BML_Expr *result = base;
    for (unsigned i = 1; result && i < exponent; i++)
      result = bml_multiply(ctx, result, base);
    return result;
  }
  if (!expr->child_count)
    return canonical(ctx, expr);
  BML_Expr *left = expand_small_powers(ctx, expr->children[0]);
  if (!left)
    return NULL;
  if (expr->type == BML_NODE_NEGATE)
    return bml_negate(ctx, left);
  if (expr->type == BML_NODE_FUNCTION)
    return bml_function(ctx, expr->value.identifier, left);
  BML_Expr *right = expand_small_powers(ctx, expr->children[1]);
  if (!right)
    return NULL;
  switch (expr->type) {
  case BML_NODE_ADD:
    return bml_add(ctx, left, right);
  case BML_NODE_SUBTRACT:
    return bml_subtract(ctx, left, right);
  case BML_NODE_MULTIPLY:
    return bml_multiply(ctx, left, right);
  case BML_NODE_DIVIDE:
    return bml_divide(ctx, left, right);
  case BML_NODE_POWER:
    return bml_power(ctx, left, right);
  default:
    return NULL;
  }
}

static size_t count_cse(const BML_Expr *expr, const BML_Expr **seen, size_t *counts,
                        size_t *used, size_t capacity) {
  if (!expr)
    return 0;
  for (size_t i = 0; i < *used; i++)
    if (bml_expr_equal(seen[i], expr)) {
      counts[i]++;
      goto children;
    }
  if (*used < capacity) {
    seen[*used] = expr;
    counts[(*used)++] = 1;
  }
children:
  for (size_t i = 0; i < expr->child_count; i++)
    count_cse(expr->children[i], seen, counts, used, capacity);
  size_t total = 0;
  for (size_t i = 0; i < *used; i++)
    if (counts[i] > 1 && seen[i]->child_count)
      total++;
  return total;
}

static size_t cse_savings(const BML_Expr *expr) {
  const BML_Expr *seen[128];
  size_t counts[128], used = 0;
  count_cse(expr, seen, counts, &used, 128);
  size_t savings = 0;
  for (size_t i = 0; i < used; i++)
    if (counts[i] > 1 && seen[i]->child_count) {
      BML_OperationCost cost;
      bml_analyze_cost(seen[i], &cost);
      savings += (counts[i] - 1) * cost.estimated_c_operations;
    }
  return savings;
}

static int optimize_local(BML_Context *ctx, const BML_Expr *expr,
                          BML_OptimizationTarget target, BML_OptimizerResult *result) {
  if (!ctx || !expr || !result || target > BML_OPTIMIZE_C_RUNTIME)
    return 0;
  memset(result, 0, sizeof(*result));
  BML_Expr *canonical_expr = bml_simplify(ctx, expr);
  BML_Expr *expanded = expand_small_powers(ctx, canonical_expr);
  if (!canonical_expr || !expanded)
    return 0;
  result->canonical = canonical_expr;
  result->target = target;
  result->candidate_count = 2;
  result->candidates[0] = (BML_OptimizationCandidate){"power", canonical_expr, {0}, 0};
  result->candidates[1] =
      (BML_OptimizationCandidate){"repeated-multiply", expanded, {0}, 0};
  for (size_t i = 0; i < result->candidate_count; i++) {
    BML_OptimizationCandidate *candidate = &result->candidates[i];
    bml_analyze_cost(candidate->expression, &candidate->cost);
    candidate->score = target == BML_OPTIMIZE_READABILITY ? candidate->cost.nodes
                       : target == BML_OPTIMIZE_OPERATION_COUNT
                           ? candidate->cost.mathematical_operations
                           : candidate->cost.estimated_c_operations;
    if (target == BML_OPTIMIZE_C_RUNTIME)
      candidate->score -= cse_savings(candidate->expression);
  }
  if (target == BML_OPTIMIZE_READABILITY)
    result->selected_candidate = 0;
  else if (result->candidates[1].score < result->candidates[0].score)
    result->selected_candidate = 1;
  result->selected = result->candidates[result->selected_candidate].expression;
  const BML_Expr *seen[128];
  size_t counts[128], used = 0;
  result->cse_count = count_cse(result->selected, seen, counts, &used, 128);
  return 1;
}

int bml_optimize_with_engine(BML_Context *ctx, const BML_Expr *expr,
                             BML_OptimizationTarget target, BML_OptimizerEngine engine,
                             BML_OptimizerResult *result) {
  if (engine > BML_OPTIMIZER_EGRAPH || !optimize_local(ctx, expr, target, result))
    return 0;

  result->engine = engine;
  if (engine != BML_OPTIMIZER_EGRAPH)
    return 1;

  const char *proof_steps = NULL;
  BML_Expr *candidate =
      bml_egraph_explore(ctx, result->canonical, &result->egraph, &proof_steps);
  result->proof_steps = proof_steps;
  if (!candidate)
    return 1;

  BML_OptimizationCandidate *alternative =
      &result->candidates[result->candidate_count++];
  *alternative = (BML_OptimizationCandidate){"egraph", candidate, {0}, 0};
  bml_analyze_cost(candidate, &alternative->cost);
  alternative->score = target == BML_OPTIMIZE_READABILITY ? alternative->cost.nodes
                       : target == BML_OPTIMIZE_OPERATION_COUNT
                           ? alternative->cost.mathematical_operations
                           : alternative->cost.estimated_c_operations;
  if (target == BML_OPTIMIZE_C_RUNTIME)
    alternative->score -= cse_savings(candidate);
  if (alternative->score < result->candidates[result->selected_candidate].score) {
    result->selected_candidate = result->candidate_count - 1;
    result->selected = candidate;
  }
  return 1;
}

int bml_optimize(BML_Context *ctx, const BML_Expr *expr, BML_OptimizationTarget target,
                 BML_OptimizerResult *result) {
  return bml_optimize_with_engine(ctx, expr, target, BML_OPTIMIZER_LOCAL, result);
}
