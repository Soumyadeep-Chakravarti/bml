#ifndef BML_AST_BUILDER_H
#define BML_AST_BUILDER_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================
 * Literals
 *==========================================================*/

BML_Expr *bml_number(BML_Context *ctx, double value);

BML_Expr *bml_variable(BML_Context *ctx, const char *name);

BML_Expr *bml_constant(BML_Context *ctx, const char *name);

/*==========================================================
 * Unary Operators
 *==========================================================*/

BML_Expr *bml_negate(BML_Context *ctx, BML_Expr *operand);

/*==========================================================
 * Binary Operators
 *==========================================================*/

BML_Expr *bml_add(BML_Context *ctx, BML_Expr *left, BML_Expr *right);

BML_Expr *bml_subtract(BML_Context *ctx, BML_Expr *left, BML_Expr *right);

BML_Expr *bml_multiply(BML_Context *ctx, BML_Expr *left, BML_Expr *right);

BML_Expr *bml_divide(BML_Context *ctx, BML_Expr *left, BML_Expr *right);

BML_Expr *bml_power(BML_Context *ctx, BML_Expr *base, BML_Expr *exponent);

#ifdef __cplusplus
}
#endif

#endif /* BML_AST_BUILDER_H */
