#ifndef clox_vm_h
#define clox_vm_h

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * (UINT8_MAX + 1))

#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"

#include <stdint.h>

// Represents an ongoing function call.
typedef struct {
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots; // points at the bottom of the function's value stack
} CallFrame;

// Virtual machine
typedef struct {
    Chunk *chunk;
    
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stackTop; // points to where the next pushed value goes

    Obj *objects; // linked-list of all heap-allocated values

    ObjUpvalue *openUpvalues; // linked-list of open upvalues

    Table strings; // for string interning
    Table globals; // for global variables

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    int grayCount;
    int grayCapacity;
    Obj **grayStack;

    size_t bytesAllocated; // number of bytes the VM has allocated for live objects
    size_t nextGC; // threshold for next GC trigger
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