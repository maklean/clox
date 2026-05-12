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

void writeConstant(Chunk* chunk, Value value, int line) {
    writeValueArray(&chunk->constants, value);

    int index = chunk->constants.count-1;

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
}