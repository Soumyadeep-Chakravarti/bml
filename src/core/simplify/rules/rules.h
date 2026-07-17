#ifndef BML_INTERNAL_RULES_H
#define BML_INTERNAL_RULES_H

#include "bml/core/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================
 * Addition Rules
 *==========================================================*/

BML_Expr *rule_add_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

BML_Expr *rule_add_self(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

BML_Expr *rule_fold_add(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

/*==========================================================
 * Subtraction Rules
 *==========================================================*/

BML_Expr *rule_sub_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

BML_Expr *rule_sub_self(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

BML_Expr *rule_fold_sub(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

/*==========================================================
 * Negation Rules
 *==========================================================*/

BML_Expr *rule_cancel_negation(BML_Context *ctx, const BML_Expr *left);

/*==========================================================
 * Multiplication Rules
 *==========================================================*/

BML_Expr *rule_mul_one(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right);

BML_Expr *rule_mul_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

BML_Expr *rule_fold_mul(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

/*==========================================================
 * Division Rules
 *==========================================================*/

BML_Expr *rule_div_one(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right);

BML_Expr *rule_fold_div(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

/*==========================================================
 * Power Rules
 *==========================================================*/

BML_Expr *rule_pow_zero(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

BML_Expr *rule_pow_one(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right);

BML_Expr *rule_one_pow(BML_Context *ctx, const BML_Expr *left,
                       const BML_Expr *right);

BML_Expr *rule_fold_pow(BML_Context *ctx, const BML_Expr *left,
                        const BML_Expr *right);

#ifdef __cplusplus
}
#endif

#endif /* BML_INTERNAL_RULES_H */
