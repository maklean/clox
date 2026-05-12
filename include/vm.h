#ifndef clox_vm_h
#define clox_vm_h

#define STACK_MAX 256

#include "chunk.h"
#include "value.h"

#include <stdint.h>

// Virtual machine
typedef struct {
    Chunk *chunk;
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stackTop; // points to where the next pushed value goes
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

// Initializes the VM.
void initVM();

void freeVM();

// Interprets the given Chunk of instructions.
InterpretResult interpret(Chunk *chunk);

// Pushes a value onto the VM's stack.
void push(Value value);

// Pops a value from the VM's stack.
Value pop();

#endif