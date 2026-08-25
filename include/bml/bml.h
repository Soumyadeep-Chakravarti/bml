#ifndef BML_H
#define BML_H

/* Stable umbrella header for the supported C API. */
#include "version.h"

/* Core ownership, AST construction, validation, and transformations. */
#include "core/arena.h"
#include "core/ast.h"
#include "core/ast_builder.h"
#include "core/ast_print.h"
#include "core/ast_validate.h"
#include "core/context.h"
#include "core/errors.h"
#include "core/simplify.h"
/* Scalar expression frontend, semantics, and verification. */
#include "semantics.h"
#include "showcase.h"
#include "verify.h"
#include "visitor.h"

#endif
