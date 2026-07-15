#include "bml/core/context.h"

#include <stdlib.h>

BML_Context *bml_context_create(void) {
  BML_Context *ctx = malloc(sizeof(BML_Context));

  if (!ctx)
    return NULL;

  ctx->arena = bml_arena_create();

  if (!ctx->arena) {
    free(ctx);
    return NULL;
  }

  return ctx;
}

void bml_context_destroy(BML_Context *ctx) {
  if (!ctx)
    return;

  bml_arena_destroy(ctx->arena);

  free(ctx);
}
