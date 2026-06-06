#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Opcode enum
typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_POP,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_PRINT,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_GET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL,
    OP_SET_LOCAL_LONG,
    OP_GET_UPVALUE,
    OP_GET_UPVALUE_LONG,
    OP_SET_UPVALUE,
    OP_SET_UPVALUE_LONG,
    OP_CLOSE_UPVALUE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSURE_LONG,
    OP_RETURN
} OpCode;

// Dynamic array of bytecode instructions
typedef struct {
    int count;
    int capacity;
    uint8_t *code; // array of bytes
    int *lines; // array of line numbers corresponding to 'code'
    ValueArray constants; // constant pool
} Chunk;

// Initializes a Chunk data structure (initializes to a completely empty array).
void initChunk(Chunk *chunk);

// Writes to the given Chunk dynamic array.
void writeChunk(Chunk* chunk, uint8_t byte, int line);

// Writes the given constant to the Chunk's constant pool, and writes to the Chunk's dynamic bytecode array. Returns whether there was enough space to write.
bool writeConstant(Chunk* chunk, Value value, int line);

// Frees the Chunks dynamic memory from the heap.
void freeChunk(Chunk* chunk);

#endif