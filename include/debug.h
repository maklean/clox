#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

// Disassembles/prints all the instructions in a given chunk.
void disassembleChunk(Chunk *chunk, const char *name);

// Disassembles/prints a single instruction. Returns the next offset in the instruction set.
int disassembleInstruction(Chunk *chunk, int offset);

#endif