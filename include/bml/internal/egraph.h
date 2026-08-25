#ifndef BML_INTERNAL_EGRAPH_H
#define BML_INTERNAL_EGRAPH_H

#include "bml/core/simplify.h"

BML_Expr *bml_egraph_explore(BML_Context *ctx, const BML_Expr *expr,
                             BML_EGraphMetrics *metrics, const char **proof_steps);

#endif
