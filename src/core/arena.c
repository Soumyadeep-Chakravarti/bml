#include "bml/core/arena.h"

#include <stdlib.h>
#include <string.h>

#define BML_DEFAULT_ARENA_SIZE 4096

typedef struct BML_ArenaBlock {
  unsigned char *memory;
  size_t capacity;
  size_t offset;
  struct BML_ArenaBlock *next;
} BML_ArenaBlock;

struct BML_Arena {
  BML_ArenaBlock *blocks;
};

static BML_ArenaBlock *bml_arena_block_create(size_t capacity) {
  BML_ArenaBlock *block = malloc(sizeof(*block));
  if (!block)
    return NULL;

  block->memory = malloc(capacity);
  if (!block->memory) {
    free(block);
    return NULL;
  }

  block->capacity = capacity;
  block->offset = 0;
  block->next = NULL;
  return block;
}

BML_Arena *bml_arena_create(void) {
  BML_Arena *arena = malloc(sizeof(*arena));

  if (!arena)
    return NULL;

  arena->blocks = bml_arena_block_create(BML_DEFAULT_ARENA_SIZE);
  if (!arena->blocks) {
    free(arena);
    return NULL;
  }

  return arena;
}

void *bml_arena_alloc(BML_Arena *arena, size_t size) {
  if (!arena || size == 0)
    return NULL;

  BML_ArenaBlock *block = arena->blocks;
  size_t alignment = _Alignof(max_align_t);
  size_t padding = (alignment - (block->offset % alignment)) % alignment;

  if (padding > block->capacity - block->offset ||
      size > block->capacity - block->offset - padding) {
    size_t new_capacity = block->capacity * 2;

    while (new_capacity < size)
      new_capacity *= 2;

    BML_ArenaBlock *new_block = bml_arena_block_create(new_capacity);
    if (!new_block)
      return NULL;

    new_block->next = block;
    arena->blocks = new_block;
    block = new_block;
    padding = 0;
  }

  void *ptr = block->memory + block->offset + padding;

  block->offset += padding + size;

  memset(ptr, 0, size);

  return ptr;
}

void bml_arena_reset(BML_Arena *arena) {
  if (!arena)
    return;

  for (BML_ArenaBlock *block = arena->blocks; block; block = block->next)
    block->offset = 0;
}

void bml_arena_destroy(BML_Arena *arena) {
  if (!arena)
    return;

  BML_ArenaBlock *block = arena->blocks;
  while (block) {
    BML_ArenaBlock *next = block->next;
    free(block->memory);
    free(block);
    block = next;
  }

  free(arena);
}
