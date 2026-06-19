#include "../include/chunk.h"
#include "../include/memory.h"
#include "../include/vm.h"

#include <stdlib.h>
#include <stdbool.h>

void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    
    #ifdef INLINE_CACHING
    chunk->cache = NULL;
    chunk->cacheCount = 0;
    chunk->cacheCapacity = 0;
    #endif

    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(sizeof(uint8_t), chunk->code, chunk->capacity);
    FREE_ARRAY(sizeof(int), chunk->lines, chunk->capacity);
    
    #ifdef INLINE_CACHING
    FREE_ARRAY(sizeof(InlineCache), chunk->cache, chunk->cacheCapacity);
    #endif

    // reset
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    // Grow chunk if there's not enough space allocated.
    if(chunk->count >= chunk->capacity) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = (uint8_t *)GROW_ARRAY(sizeof(uint8_t), chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = (int *)GROW_ARRAY(sizeof(int), chunk->lines, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count++] = line;
}

bool writeConstant(Chunk* chunk, Value value, int line) {
    // Push value onto the stack so it doesn't get sweeped by the GC b/c of array resize
    push(value);

    writeValueArray(&chunk->constants, value);

    pop(value);

    int index = chunk->constants.count-1;

    // Not enough space if it passes the UINT24_MAX
    if(index > 0xFFFFFF) {
        return false;
    }

    if(index <= 255) {
        // use OP_CONSTANT for indexes in [0, 255]
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, index, line);
    } else {
        // use OP_CONSTANT_LONG for indexes > 255
        writeChunk(chunk, OP_CONSTANT_LONG, line);

        // seperate into three different bytes
        writeChunk(chunk, (index >> 16) & 0xFF, line);
        writeChunk(chunk, (index >> 8) & 0xFF, line);
        writeChunk(chunk, index & 0xFF, line);
    }

    return true;
}

#ifdef INLINE_CACHING
int addCache(Chunk *chunk) {
    if(chunk->cacheCount >= UINT16_MAX) {
        return -1;
    }

    if(chunk->cacheCount >= chunk->cacheCapacity) {
        int oldCapacity = chunk->cacheCapacity;
        chunk->cacheCapacity = GROW_CAPACITY(oldCapacity);

        chunk->cache = (InlineCache *)GROW_ARRAY(sizeof(InlineCache), chunk->cache, oldCapacity, chunk->cacheCapacity);
    }

    // placeholder InlineCache for now (it'll get set at runtime if the property needs to be cached)
    chunk->cache[chunk->cacheCount] = (InlineCache){.klass = NULL, .index = -1 };

    return chunk->cacheCount++;
}
#endif