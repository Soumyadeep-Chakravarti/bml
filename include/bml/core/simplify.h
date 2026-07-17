#ifndef BML_SIMPLIFY_H
#define BML_SIMPLIFY_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

BML_Expr *bml_simplify(BML_Context *ctx, const BML_Expr *expr);

#ifdef __cplusplus
}
#endif

#endif /* BML_SIMPLIFY_H */
