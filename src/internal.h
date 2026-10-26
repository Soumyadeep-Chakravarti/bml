#ifndef BML_INTERNAL_H
#define BML_INTERNAL_H

#include "bml/bml.h"

#include <stddef.h>

typedef struct BML_ArenaBlock BML_ArenaBlock;

struct BML_Context {
    BML_ArenaBlock *blocks;
    BML_Error error;
};

struct BML_Expr {
    BML_ExprKind kind;
    size_t child_count;
    const char *name;
    const char *number_text;
    struct BML_Expr **children;
};

void *bml_arena_alloc(BML_Context *ctx, size_t size);
char *bml_arena_copy(BML_Context *ctx, const char *text, size_t length);
void bml_set_error(BML_Context *ctx, BML_ErrorCode code, size_t offset);
BML_Expr *bml_expr_create(BML_Context *ctx, BML_ExprKind kind,
                          const char *name, const char *number_text,
                          BML_Expr *const *children, size_t child_count);

#endif
