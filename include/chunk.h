#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Opcode enum
typedef enum {
    OP_CONSTANT,
    OP_RETURN
} OpCode;

// Dynamic array of bytecode instructions
typedef struct {
    int count;
    int capacity;
    uint8_t *code; // array of bytes
    int* lines; // array of line numbers corresponding to 'code'
    ValueArray constants; // constant pool
} Chunk;

// Initializes a Chunk data structure (initializes to a completely empty array).
void initChunk(Chunk *chunk);

// Writes to the given Chunk dynamic array.
void writeChunk(Chunk* chunk, uint8_t byte, int line);

// Adds a constant to the Chunk's constant pool. Returns the index where the constant was appended in the constant pool.
int addConstant(Chunk *chunk, Value value);

// Frees the Chunks dynamic memory from the heap.
void freeChunk(Chunk* chunk);

#endif