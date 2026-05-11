#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

// Disassembles all the instructions in a given chunk.
void disassembleChunk(Chunk *chunk, const char *name);

// Disassembles a single instruction. Returns the next offset in the instruction set.
int disassembleInstruction(Chunk *chunk, int offset);

#endif