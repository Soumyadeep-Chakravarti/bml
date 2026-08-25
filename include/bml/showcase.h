#ifndef BML_SHOWCASE_H
#define BML_SHOWCASE_H

#include <stddef.h>

#include "core/ast.h"
#include "core/errors.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *name;
  double value;
} BML_Binding;

/* Parses a scalar real expression. On failure, error is BML_ERROR_PARSE. */
BML_Expr *bml_parse(BML_Context *ctx, const char *input, BML_Error *error);

BML_Error bml_evaluate(const BML_Expr *expr, const BML_Binding *bindings,
                       size_t binding_count, double *result);
BML_Expr *bml_substitute(BML_Context *ctx, const BML_Expr *expr, const char *name,
                         const BML_Expr *replacement);
BML_Expr *bml_differentiate(BML_Context *ctx, const BML_Expr *expr,
                            const char *variable);

/* Writes a NUL-terminated result when buffer_size is nonzero. */
BML_Error bml_format_infix(const BML_Expr *expr, char *buffer, size_t buffer_size);
BML_Error bml_emit_c(const BML_Expr *expr, char *buffer, size_t buffer_size);

/* Emits a complete C function, introducing temporaries for repeated non-leaf trees. */
BML_Error bml_emit_c_optimized(const BML_Expr *expr, const char *function_name,
                               const char *const *parameters, size_t parameter_count,
                               char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
