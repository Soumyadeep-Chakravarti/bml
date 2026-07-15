#ifndef BML_CORE_CONTEXT_H
#define BML_CORE_CONTEXT_H

#include "arena.h"

#ifdef __cplusplus

extern "C" {

#endif

typedef struct BML_Context {
  BML_Arena *arena;

} BML_Context;

BML_Context *bml_context_create(void);

void bml_context_destroy(BML_Context *ctx);

#ifdef __cplusplus
}

#endif

#endif
