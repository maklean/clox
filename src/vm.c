#include "../include/common.h"
#include "../include/debug.h"
#include "../include/vm.h"
#include "../include/compiler.h"

#include <stdio.h>

VM vm;

static InterpretResult run();
static void resetStack();

void initVM() {
    resetStack();
}

void freeVM() {

}

static InterpretResult run() {
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define READ_CONSTANT_LONG() (vm.chunk->constants.values[(READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE()])
    #define BINARY_OP(op) \
        do { \
            double b = pop(); \
            double a = pop(); \
            push(a op b); \
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
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }

            case OP_CONSTANT:
            case OP_CONSTANT_LONG: {
                Value constant = instruction == OP_CONSTANT ? READ_CONSTANT() : READ_CONSTANT_LONG();
                push(constant);
                break;
            }

            case OP_ADD: BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;

            case OP_NEGATE: push(-pop()); break;
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_CONSTANT_LONG
    #undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    compile(source);
    return INTERPRET_OK;
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