#ifndef BML_BML_H
#define BML_BML_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BML_Context BML_Context;
typedef struct BML_Expr BML_Expr;

typedef enum {
    BML_EXPR_INVALID,
    BML_EXPR_NUMBER,
    BML_EXPR_SYMBOL,
    BML_EXPR_ADD,
    BML_EXPR_SUBTRACT,
    BML_EXPR_MULTIPLY,
    BML_EXPR_DIVIDE,
    BML_EXPR_POWER,
    BML_EXPR_NEGATE,
    BML_EXPR_EQUAL,
    BML_EXPR_APPROACHES,
    BML_EXPR_CALL
} BML_ExprKind;

typedef enum {
    BML_ERROR_NONE,
    BML_ERROR_INVALID_ARGUMENT,
    BML_ERROR_PARSE,
    BML_ERROR_OUT_OF_MEMORY
} BML_ErrorCode;

typedef struct {
    BML_ErrorCode code;
    size_t offset;
} BML_Error;

/* A context owns all expressions created by its parser. */
BML_Context *bml_context_create(void);
void bml_context_destroy(BML_Context *ctx);
BML_Error bml_context_error(const BML_Context *ctx);
void bml_context_clear_error(BML_Context *ctx);

/* Parse arithmetic expressions, symbols, and function calls. */
BML_Expr *bml_parse(BML_Context *ctx, const char *input);

/* Read-only expression inspection. Returned strings live as long as ctx. */
BML_ExprKind bml_expr_kind(const BML_Expr *expr);
size_t bml_expr_child_count(const BML_Expr *expr);
const BML_Expr *bml_expr_child(const BML_Expr *expr, size_t index);
const char *bml_expr_name(const BML_Expr *expr);
const char *bml_expr_number_text(const BML_Expr *expr);

#ifdef __cplusplus
}
#endif

#endif
