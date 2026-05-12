#include "../include/common.h"
#include "../include/chunk.h"
#include "../include/debug.h"
#include "../include/vm.h"

#include <stdio.h>

int main(int argc, const char **argv) {
    initVM();
    Chunk chunk;

    initChunk(&chunk);

    // perform: 1 + 2 * 3 - 4 / -5

    // 2*3
    writeConstant(&chunk, 2, 1);
    writeConstant(&chunk, 3, 1);
    writeChunk(&chunk, OP_MULTIPLY, 1);

    // +1
    writeConstant(&chunk, 1, 1);
    writeChunk(&chunk, OP_ADD, 1);

    // 4 / -5
    writeConstant(&chunk, 4, 1);
    writeConstant(&chunk, 5, 1);
    writeChunk(&chunk, OP_NEGATE, 1);
    writeChunk(&chunk, OP_DIVIDE, 1);
    
    writeChunk(&chunk, OP_SUBTRACT, 1);

    writeChunk(&chunk, OP_RETURN, 1);

    printf("Interpret result = %d\n", interpret(&chunk));

    freeChunk(&chunk);
    freeVM();

    return 0;
}