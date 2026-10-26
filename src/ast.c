#include "internal.h"

#include <stdint.h>
#include <string.h>

BML_Expr *bml_expr_create(BML_Context *ctx, BML_ExprKind kind,
                          const char *name, const char *number_text,
                          BML_Expr *const *children, size_t child_count) {
    BML_Expr *expr = bml_arena_alloc(ctx, sizeof(*expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->kind = kind;
    expr->child_count = child_count;
    expr->name = name;
    expr->number_text = number_text;
    expr->children = NULL;

    if (child_count != 0) {
        if (child_count > SIZE_MAX / sizeof(*expr->children)) {
            bml_set_error(ctx, BML_ERROR_OUT_OF_MEMORY, 0);
            return NULL;
        }
        expr->children = bml_arena_alloc(ctx, child_count * sizeof(*expr->children));
        if (expr->children == NULL) {
            return NULL;
        }
        memcpy(expr->children, children, child_count * sizeof(*expr->children));
    }
    return expr;
}

BML_ExprKind bml_expr_kind(const BML_Expr *expr) {
    return expr == NULL ? BML_EXPR_INVALID : expr->kind;
}

size_t bml_expr_child_count(const BML_Expr *expr) {
    return expr == NULL ? 0 : expr->child_count;
}

const BML_Expr *bml_expr_child(const BML_Expr *expr, size_t index) {
    if (expr == NULL || index >= expr->child_count) {
        return NULL;
    }
    return expr->children[index];
}

const char *bml_expr_name(const BML_Expr *expr) {
    return expr == NULL ? NULL : expr->name;
}

const char *bml_expr_number_text(const BML_Expr *expr) {
    return expr == NULL ? NULL : expr->number_text;
}
