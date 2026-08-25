#ifndef BML_VERIFY_H
#define BML_VERIFY_H

#include <stddef.h>

#include "core/simplify.h"
#include "showcase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const BML_Binding *bindings; /* Optional caller-provided bindings. */
  size_t binding_count;
  size_t trials;             /* Generated trials when bindings is NULL; default 32. */
  double absolute_tolerance; /* Default 1e-9. */
} BML_VerifyOptions;

typedef struct {
  size_t trials;
  size_t passed;
  double max_absolute_error;
  size_t first_failure; /* SIZE_MAX when all trials pass. */
  BML_Error error;
} BML_VerificationResult;

/* Compares scalar evaluations at supplied bindings or deterministic valid samples. */
int bml_verify_equivalent(const BML_Context *ctx, const BML_Expr *original,
                          const BML_Expr *candidate, const BML_VerifyOptions *options,
                          BML_VerificationResult *result);

/* Checks symbolic derivative against centered finite differences at valid samples. */
int bml_verify_derivative(const BML_Context *ctx, const BML_Expr *expr,
                          const BML_Expr *derivative, const char *variable,
                          const BML_VerifyOptions *options,
                          BML_VerificationResult *result);

#ifdef __cplusplus
}
#endif
#endif
