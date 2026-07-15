#ifndef BML_CORE_ARENA_H
#define BML_CORE_ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BML_Arena BML_Arena;

BML_Arena *bml_arena_create(void);

void *bml_arena_alloc(BML_Arena *arena, size_t size);

void bml_arena_reset(BML_Arena *arena);

void bml_arena_destroy(BML_Arena *arena);

#ifdef __cplusplus
}
#endif

#endif
