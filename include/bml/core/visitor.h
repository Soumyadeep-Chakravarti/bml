#ifndef BML_CORE_VISITOR_H
#define BML_CORE_VISITOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct BML_Expr;

typedef void (*BML_Visitor)(struct BML_Expr *expr, void *userdata);

void bml_visit(struct BML_Expr *root, BML_Visitor visitor, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
