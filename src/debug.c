#include "../include/debug.h"
#include "../include/value.h"

#include <stdio.h>
#include <string.h>

// Displays a simple instruction to the console. Offsets to the immediate next instruction.
static int simpleInstruction(const char *name, int offset);

// Displays a constant instruction to the console.
static int constantInstruction(const char *name, Chunk *chunk, int offset);

void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    // Iterate over every instruction
    for(int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

int disassembleInstruction(Chunk *chunk, int offset) {
    // print where the current instruction is
    printf("%04d ", offset);

    // print where the line is
    if(offset > 0 && chunk->lines[offset] == chunk->lines[offset-1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    // print instruction at 'offset'
    uint8_t instruction = chunk->code[offset];
    switch(instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constantInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset+1;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    int constant; // constant index
    int offset_skip;

    if(strcmp(name, "OP_CONSTANT") == 0) {
        constant = chunk->code[offset+1];
        offset_skip = 2;
    } else {
        // OP_CONSTANT_LONG get 3 bytes for index
        constant = (chunk->code[offset+1] << 16) | (chunk->code[offset+2] << 8) | chunk->code[offset+3];
        offset_skip = 4;
    }

    // print instruction name + constant
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    // skip over instruction + const. index operand
    return offset+offset_skip;
}