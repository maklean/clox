#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Opcode enum
typedef enum {
    OP_CONSTANT, // 1 byte operand
    OP_CONSTANT_LONG, // 3 byte operand
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

// Writes the given constant to the Chunk's constant pool, and writes to the Chunk's dynamic bytecode array.
void writeConstant(Chunk* chunk, Value value, int line);

// Frees the Chunks dynamic memory from the heap.
void freeChunk(Chunk* chunk);

#endif