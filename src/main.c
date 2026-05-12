#include "../include/common.h"
#include "../include/chunk.h"
#include "../include/debug.h"

int main(int argc, const char **argv) {
    Chunk chunk;

    initChunk(&chunk);

    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    for(int i = 0; i < 256; i++) writeConstant(&chunk, i, 256);

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);

    return 0;
}