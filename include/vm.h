#ifndef clox_vm_h
#define clox_vm_h

#define STACK_MAX (UINT16_MAX+1) // uint16 limit

#include "chunk.h"
#include "value.h"
#include "table.h"

#include <stdint.h>

// Virtual machine
typedef struct {
    Chunk *chunk;
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stackTop; // points to where the next pushed value goes
    Obj *objects; // linked-list of all heap-allocated values
    Table strings; // for string interning
    Table globals; // for global variables
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

// Initializes the VM.
void initVM();

void freeVM();

// Interprets the given code.
InterpretResult interpret(const char *source);

// Pushes a value onto the VM's stack.
void push(Value value);

// Pops a value from the VM's stack.
Value pop();

#endif