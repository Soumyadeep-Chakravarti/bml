#ifndef BML_SEMANTICS_H
#define BML_SEMANTICS_H

#include "core/simplify.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Conservative scalar-real facts derived from literals and named assumptions. */
uint32_t bml_expr_assumptions(const BML_Context *ctx, const BML_Expr *expr);

typedef enum {
  BML_DEFINED_UNKNOWN,
  BML_DEFINED,
  BML_UNDEFINED,
} BML_Definedness;

/* Classifies division, log, and power domains under named assumptions. */
BML_Definedness bml_expr_definedness(const BML_Context *ctx, const BML_Expr *expr);
int bml_expr_is_defined(const BML_Context *ctx, const BML_Expr *expr);

#ifdef __cplusplus
}
#endif

#endif
