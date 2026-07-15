#include "bml/core/arena.h"

#include <stdlib.h>
#include <string.h>

#define BML_DEFAULT_ARENA_SIZE 4096

struct BML_Arena {
  unsigned char *memory;

  size_t capacity;
  size_t offset;
};

BML_Arena *bml_arena_create(void) {
  BML_Arena *arena = malloc(sizeof(BML_Arena));

  if (!arena)
    return NULL;

  arena->capacity = BML_DEFAULT_ARENA_SIZE;
  arena->offset = 0;

  arena->memory = malloc(arena->capacity);

  if (!arena->memory) {
    free(arena);
    return NULL;
  }

  return arena;
}

void *bml_arena_alloc(BML_Arena *arena, size_t size) {
  if (!arena || size == 0)
    return NULL;

  if (arena->offset + size > arena->capacity) {
    size_t new_capacity = arena->capacity * 2;

    while (new_capacity < arena->offset + size)
      new_capacity *= 2;

    unsigned char *new_memory = realloc(arena->memory, new_capacity);

    if (!new_memory)
      return NULL;

    arena->memory = new_memory;
    arena->capacity = new_capacity;
  }

  void *ptr = arena->memory + arena->offset;

  arena->offset += size;

  memset(ptr, 0, size);

  return ptr;
}

void bml_arena_reset(BML_Arena *arena) {
  if (!arena)
    return;

  arena->offset = 0;
}

void bml_arena_destroy(BML_Arena *arena) {
  if (!arena)
    return;

  free(arena->memory);
  free(arena);
}
