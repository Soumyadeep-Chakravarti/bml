#ifndef BML_INTERNAL_AST_HELPERS_H
#define BML_INTERNAL_AST_HELPERS_H

#include "bml/core/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

BML_Expr *bml_node(BML_Context *ctx, BML_NodeType type, size_t child_count);

BML_Expr *bml_unary(BML_Context *ctx, BML_NodeType type, BML_Expr *child);

BML_Expr *bml_binary(BML_Context *ctx, BML_NodeType type, BML_Expr *left,
                     BML_Expr *right);

#ifdef __cplusplus
}
#endif

#endif /* BML_INTERNAL_AST_HELPERS_H */
