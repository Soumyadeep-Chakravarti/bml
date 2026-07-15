#ifndef BML_CORE_AST_VALIDATE_H
#define BML_CORE_AST_VALIDATE_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BML_VALID = 0,

  BML_INVALID_NULL_NODE,

  BML_INVALID_CHILD_COUNT,

  BML_INVALID_NODE_TYPE

} BML_ValidationError;

BML_ValidationError bml_validate_ast(const BML_Expr *expr);

const char *bml_validation_error_string(BML_ValidationError error);

#ifdef __cplusplus
}
#endif

#endif
