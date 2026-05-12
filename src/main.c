#include "../include/common.h"
#include "../include/chunk.h"
#include "../include/debug.h"

int main(int argc, const char **argv) {
    Chunk chunk;

    initChunk(&chunk);
    
    writeConstant(&chunk, 1.2, 123);

    for(int i = 0; i < 256; i++) writeConstant(&chunk, i, 256);

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);

    return 0;
}