#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.h"

#include <stdbool.h>

// Compiles the source code and inserts the bytecode instructions into the chunk.
bool compile(const char* source, Chunk *chunk);

#endif
