#include "internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct BML_ArenaBlock {
    struct BML_ArenaBlock *next;
    size_t used;
    size_t capacity;
    max_align_t alignment;
    unsigned char data[];
};

static size_t align_size(size_t size) {
    const size_t alignment = _Alignof(max_align_t);
    const size_t remainder = size % alignment;
    if (remainder == 0) {
        return size;
    }
    if (size > SIZE_MAX - (alignment - remainder)) {
        return 0;
    }
    return size + alignment - remainder;
}

static BML_ArenaBlock *new_block(size_t capacity) {
    if (capacity > SIZE_MAX - sizeof(BML_ArenaBlock)) {
        return NULL;
    }
    BML_ArenaBlock *block = malloc(sizeof(*block) + capacity);
    if (block == NULL) {
        return NULL;
    }
    block->next = NULL;
    block->used = 0;
    block->capacity = capacity;
    return block;
}

BML_Context *bml_context_create(void) {
    BML_Context *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }
    return ctx;
}

void bml_context_destroy(BML_Context *ctx) {
    if (ctx == NULL) {
        return;
    }
    BML_ArenaBlock *block = ctx->blocks;
    while (block != NULL) {
        BML_ArenaBlock *next = block->next;
        free(block);
        block = next;
    }
    free(ctx);
}

BML_Error bml_context_error(const BML_Context *ctx) {
    if (ctx == NULL) {
        return (BML_Error){BML_ERROR_INVALID_ARGUMENT, 0};
    }
    return ctx->error;
}

void bml_context_clear_error(BML_Context *ctx) {
    if (ctx != NULL) {
        ctx->error = (BML_Error){BML_ERROR_NONE, 0};
    }
}

void bml_set_error(BML_Context *ctx, BML_ErrorCode code, size_t offset) {
    if (ctx != NULL && ctx->error.code == BML_ERROR_NONE) {
        ctx->error = (BML_Error){code, offset};
    }
}

void *bml_arena_alloc(BML_Context *ctx, size_t size) {
    if (ctx == NULL || size == 0) {
        return NULL;
    }

    const size_t aligned_size = align_size(size);
    if (aligned_size == 0) {
        bml_set_error(ctx, BML_ERROR_OUT_OF_MEMORY, 0);
        return NULL;
    }

    BML_ArenaBlock *block = ctx->blocks;
    if (block == NULL || aligned_size > block->capacity - block->used) {
        const size_t capacity = aligned_size > 4096 ? aligned_size : 4096;
        BML_ArenaBlock *newest = new_block(capacity);
        if (newest == NULL) {
            bml_set_error(ctx, BML_ERROR_OUT_OF_MEMORY, 0);
            return NULL;
        }
        newest->next = ctx->blocks;
        ctx->blocks = newest;
        block = newest;
    }

    void *memory = block->data + block->used;
    block->used += aligned_size;
    return memory;
}

char *bml_arena_copy(BML_Context *ctx, const char *text, size_t length) {
    if (text == NULL || length == SIZE_MAX) {
        bml_set_error(ctx, BML_ERROR_INVALID_ARGUMENT, 0);
        return NULL;
    }
    char *copy = bml_arena_alloc(ctx, length + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}
