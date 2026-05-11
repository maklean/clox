#include "../include/chunk.h"
#include "../include/memory.h"

#include <stdlib.h>

void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;

    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(sizeof(uint8_t), chunk->code, chunk->capacity);
    FREE_ARRAY(sizeof(int), chunk->lines, chunk->capacity);

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

int addConstant(Chunk *chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}