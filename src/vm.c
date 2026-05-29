#include "../include/common.h"
#include "../include/debug.h"
#include "../include/vm.h"
#include "../include/compiler.h"
#include "../include/value.h"
#include "../include/memory.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

VM vm;

static InterpretResult run();
static void resetStack();

// Peeks `distance` away from the top of the stack.
static Value peek(int distance);

// Prints a runtime error with the given formatted message.
static void runtimeError(const char *format, ...);

// Returns whether the value is falsey or not (is nil or false)
static bool isFalsey(Value value);

// Concatenates the top two strings on the stack.
static void concatenate();

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
}

void freeVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

static InterpretResult run() {
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define READ_CONSTANT_LONG() (vm.chunk->constants.values[(READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE()])
    #define READ_STRING() AS_STRING(READ_CONSTANT())
    #define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())
    #define BINARY_OP(valueType, op) \
        do { \
            if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
                runtimeError("Expected two number operands at the top of the stack. Got types: %d and %d.", peek(0).type, peek(1).type); \
                return INTERPRET_RUNTIME_ERROR; \
            } \
            double b = AS_NUMBER(pop()); \
            double a = AS_NUMBER(pop()); \
            push(valueType(a op b)); \
        } while(0)

    for(;;) {
        #ifdef DEBUG_TRACE_EXECUTION
            // print stack
            printf("          ");

            for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }

            printf("\n");

            disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif

        uint8_t instruction;

        switch(instruction = READ_BYTE()) {
            case OP_RETURN: {
                return INTERPRET_OK;
            }

            case OP_CONSTANT:
            case OP_CONSTANT_LONG: {
                Value constant = instruction == OP_CONSTANT ? READ_CONSTANT() : READ_CONSTANT_LONG();
                push(constant);
                break;
            }

            case OP_ADD: {
                if(IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    BINARY_OP(NUMBER_VAL, +);
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;

            case OP_NEGATE:
                // Check if the value at the top of the stack is a number.
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            
            case OP_TRUE: case OP_FALSE: push(BOOL_VAL(instruction == OP_TRUE)); break;
            case OP_NIL: push(NIL_VAL); break;

            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            
            case OP_PRINT:
                Value expr = pop();
                printValue(expr);
                printf("\n");
                break;
            
            case OP_POP: pop(); break;

            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                ObjString *name = instruction == OP_DEFINE_GLOBAL ? READ_STRING() : READ_STRING_LONG();
                
                // set global init. value as the item at the top of the stack
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            
            case OP_GET_GLOBAL:
            case OP_GET_GLOBAL_LONG: {
                ObjString *name = instruction == OP_GET_GLOBAL ? READ_STRING() : READ_STRING_LONG();
                Value value;

                // get global value
                if(!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(value);
                break;
            }
            case OP_SET_GLOBAL:
            case OP_SET_GLOBAL_LONG: {
                ObjString *name = instruction == OP_SET_GLOBAL ? READ_STRING() : READ_STRING_LONG();

                if(tableSet(&vm.globals, name, peek(0))) {
                    // if true is returned, an insertion happened, so the user has peformed assignment on an undefined variable.
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                break;
            }

            case OP_GET_LOCAL:
            case OP_GET_LOCAL_LONG: {
                int slot = instruction == OP_GET_LOCAL ? READ_BYTE() : ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE());
                push(vm.stack[slot]); // the slot should match the slot in the locals array
                break;
            }
            
            case OP_SET_LOCAL:
            case OP_SET_LOCAL_LONG: {
                int slot = instruction == OP_SET_LOCAL ? READ_BYTE() : ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE());
                vm.stack[slot] = peek(0); // overwrite the value at the slot with the new value
                break;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_CONSTANT_LONG
    #undef READ_STRING
    #undef READ_STRING_LONG
    #undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);

    // Compile source and write instructions into 'chunk'.
    if(!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    // Run VM with the resulting chunk instructions.
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}

static void resetStack() {
    vm.stackTop = vm.stack;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-distance - 1]; // -1 b/c stackTop is positioned where the next value goes.
}

static void runtimeError(const char *format, ...) {
    // print the formatted string
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // find instruction index in chunk and print error message
    size_t instruction = vm.ip - vm.chunk->code - 1; 
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    resetStack();
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && AS_BOOL(value) == false);
}

static void concatenate() {
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());

    int length = a->length + b->length;
    char *chars = ALLOCATE(sizeof(char), length+1);

    // copy characters from a, b, then add null-terminator
    memcpy(chars, a->chars, a->length);
    memcpy(chars+a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);

    push(OBJ_VAL(result));
}