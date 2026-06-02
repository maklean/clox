#include "../include/debug.h"
#include "../include/value.h"

#include <stdio.h>
#include <string.h>

// Displays a simple instruction to the console. Offsets to the immediate next instruction.
static int simpleInstruction(const char *name, int offset);

// Displays a constant instruction to the console.
static int constantInstruction(const char *name, Chunk *chunk, int offset);

// Displays the slot number of the instruction to the console.
static int byteInstruction(const char *name, Chunk *chunk, int offset);

// Displays a jump instruction (with its operands) to the console.
static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset);

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
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL_LONG:
            return constantInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL_LONG:
            return constantInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL_LONG:
            return constantInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_GET_LOCAL_LONG:
            return byteInstruction("OP_GET_LOCAL_LONG", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_SET_LOCAL_LONG:
            return byteInstruction("OP_SET_LOCAL_LONG", chunk, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
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

    if(
        strcmp(name, "OP_CONSTANT") == 0 || 
        strcmp(name, "OP_DEFINE_GLOBAL") == 0 ||
        strcmp(name, "OP_GET_GLOBAL") == 0 ||
        strcmp(name, "OP_SET_GLOBAL") == 0
    ) {
        constant = chunk->code[offset+1];
        offset_skip = 2;
    } else {
        // OP_CONSTANT_LONG/OP_DEFINE_GLOBAL_LONG/OP_GET_GLOBAL_LONG get 3 bytes for index
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

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    int slot;
    int offset_skip;

    if(
        strcmp(name, "OP_GET_LOCAL") == 0 || 
        strcmp(name, "OP_SET_LOCAL") == 0
    ) {
        slot = chunk->code[offset+1];
        offset_skip = 2;
    } else {
        slot = (chunk->code[offset+1] << 16) | (chunk->code[offset+2] << 8) | chunk->code[offset+3];
        offset_skip = 4;
    }

    printf("%-16s %4d\n", name, slot);

    return offset+offset_skip;
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
    uint16_t jump = (uint16_t)((chunk->code[offset+1] << 8) | chunk->code[offset+2]);
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);

    return offset + 3;
}