#ifndef BML_CORE_CONTEXT_H
#define BML_CORE_CONTEXT_H

#include "arena.h"

#ifdef __cplusplus

extern "C" {

#endif

typedef struct BML_Context {
  BML_Arena *arena;
  struct BML_SymbolAssumption *assumptions;
  size_t assumption_count;
  size_t assumption_capacity;
  struct BML_ExprMemoEntry *canonical_memo;
  size_t canonical_memo_count;
  size_t canonical_memo_capacity;
} BML_Context;

BML_Context *bml_context_create(void);

void bml_context_destroy(BML_Context *ctx);

#ifdef __cplusplus
}

#endif

#endif
