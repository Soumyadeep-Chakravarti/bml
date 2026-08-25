#include "bml/verify.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static void variables(const BML_Expr *expr, const char **names, size_t *count) {
  if (expr->type == BML_NODE_VARIABLE) {
    for (size_t i = 0; i < *count; i++)
      if (!strcmp(names[i], expr->value.identifier))
        return;
    if (*count < 64)
      names[(*count)++] = expr->value.identifier;
  }
  for (size_t i = 0; i < expr->child_count; i++)
    variables(expr->children[i], names, count);
}

static double sample(uint32_t *state, uint32_t flags) {
  *state = *state * 1664525u + 1013904223u;
  double value = ((double)(*state >> 8) / 16777216.0) * 6.0 - 3.0;
  if (flags & BML_ASSUME_POSITIVE)
    return fabs(value) + 0.25;
  if (flags & BML_ASSUME_NONNEGATIVE)
    return fabs(value);
  if (flags & BML_ASSUME_NONZERO)
    return value < 0.0 ? value - 0.25 : value + 0.25;
  return value;
}

static void initialize(BML_VerificationResult *result) {
  memset(result, 0, sizeof(*result));
  result->first_failure = SIZE_MAX;
  result->error = BML_SUCCESS;
}

static int binding_satisfies(const BML_Context *ctx, const BML_Binding *binding) {
  uint32_t flags = bml_context_assumptions(ctx, binding->name);
  if (!isfinite(binding->value))
    return 0;
  if ((flags & BML_ASSUME_POSITIVE) && binding->value <= 0.0)
    return 0;
  if ((flags & BML_ASSUME_NONNEGATIVE) && binding->value < 0.0)
    return 0;
  if ((flags & BML_ASSUME_NONZERO) && binding->value == 0.0)
    return 0;
  if ((flags & BML_ASSUME_INTEGER) && floor(binding->value) != binding->value)
    return 0;
  return 1;
}

static int bindings_satisfy(const BML_Context *ctx, const BML_Binding *bindings,
                            size_t count) {
  for (size_t i = 0; i < count; i++)
    if (!binding_satisfies(ctx, &bindings[i]))
      return 0;
  return 1;
}

int bml_verify_equivalent(const BML_Context *ctx, const BML_Expr *original,
                          const BML_Expr *candidate, const BML_VerifyOptions *options,
                          BML_VerificationResult *result) {
  if (!ctx || !original || !candidate || !result)
    return 0;
  initialize(result);
  const char *names[64];
  size_t name_count = 0;
  variables(original, names, &name_count);
  variables(candidate, names, &name_count);
  size_t wanted = options && options->bindings ? 1
                  : options && options->trials ? options->trials
                                               : 32;
  double tolerance =
      options && options->absolute_tolerance ? options->absolute_tolerance : 1e-9;
  uint32_t state = 0x0b1a040u;
  for (size_t trial = 0; trial < wanted; trial++) {
    BML_Binding generated[64];
    const BML_Binding *bindings =
        options && options->bindings ? options->bindings : generated;
    size_t count = options && options->bindings ? options->binding_count : name_count;
    if (!options || !options->bindings)
      for (size_t i = 0; i < name_count; i++)
        generated[i] = (BML_Binding){
            names[i], sample(&state, bml_context_assumptions(ctx, names[i]))};
    if (!bindings_satisfy(ctx, bindings, count)) {
      result->error = BML_ERROR_INVALID_ARGUMENT;
      result->first_failure = trial;
      return 0;
    }
    double a, b;
    BML_Error left = bml_evaluate(original, bindings, count, &a);
    BML_Error right = bml_evaluate(candidate, bindings, count, &b);
    if (left != BML_SUCCESS || right != BML_SUCCESS) {
      if (options && options->bindings) {
        result->error = left != BML_SUCCESS ? left : right;
        result->first_failure = trial;
        return 0;
      }
      continue;
    }
    double error = fabs(a - b);
    if (error > result->max_absolute_error)
      result->max_absolute_error = error;
    result->trials++;
    if (error > tolerance) {
      result->first_failure = trial;
      return 0;
    }
    result->passed++;
  }
  if (!result->trials) {
    result->error = BML_ERROR_DOMAIN;
    return 0;
  }
  return 1;
}

int bml_verify_derivative(const BML_Context *ctx, const BML_Expr *expr,
                          const BML_Expr *derivative, const char *variable,
                          const BML_VerifyOptions *options,
                          BML_VerificationResult *result) {
  if (!ctx || !expr || !derivative || !variable || !result)
    return 0;
  initialize(result);
  const char *names[64];
  size_t name_count = 0;
  variables(expr, names, &name_count);
  size_t wanted = options && options->bindings ? 1
                  : options && options->trials ? options->trials
                                               : 32;
  double tolerance =
      options && options->absolute_tolerance ? options->absolute_tolerance : 1e-5;
  uint32_t state = 0x0b1a050u;
  for (size_t trial = 0; trial < wanted; trial++) {
    BML_Binding generated[64];
    BML_Binding bindings[64];
    const BML_Binding *provided =
        options && options->bindings ? options->bindings : NULL;
    size_t binding_count = provided ? options->binding_count : name_count;
    size_t index = SIZE_MAX;
    for (size_t i = 0; i < name_count; i++) {
      generated[i] = (BML_Binding){
          names[i], sample(&state, bml_context_assumptions(ctx, names[i]))};
      if (!strcmp(names[i], variable))
        index = i;
    }
    if (index == SIZE_MAX) {
      result->error = BML_ERROR_INVALID_ARGUMENT;
      return 0;
    }
    if (provided) {
      if (!bindings_satisfy(ctx, provided, binding_count)) {
        result->error = BML_ERROR_INVALID_ARGUMENT;
        result->first_failure = trial;
        return 0;
      }
      index = SIZE_MAX;
      for (size_t i = 0; i < binding_count; i++) {
        bindings[i] = provided[i];
        if (!strcmp(bindings[i].name, variable))
          index = i;
      }
      if (index == SIZE_MAX) {
        result->error = BML_ERROR_INVALID_ARGUMENT;
        return 0;
      }
    } else
      memcpy(bindings, generated, name_count * sizeof(*bindings));
    const double h = 1e-6;
    double plus, minus, symbolic;
    bindings[index].value += h;
    BML_Error a = bml_evaluate(expr, bindings, binding_count, &plus);
    bindings[index].value -= 2.0 * h;
    BML_Error b = bml_evaluate(expr, bindings, binding_count, &minus);
    bindings[index].value += h;
    BML_Error c = bml_evaluate(derivative, bindings, binding_count, &symbolic);
    if (a != BML_SUCCESS || b != BML_SUCCESS || c != BML_SUCCESS)
      continue;
    double error = fabs(symbolic - (plus - minus) / (2.0 * h));
    if (error > result->max_absolute_error)
      result->max_absolute_error = error;
    result->trials++;
    if (error > tolerance) {
      result->first_failure = trial;
      return 0;
    }
    result->passed++;
  }
  if (!result->trials) {
    result->error = BML_ERROR_DOMAIN;
    return 0;
  }
  return 1;
}
