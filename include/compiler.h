#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.h"
#include "object.h"

#include <stdbool.h>

// Compiles the source code and inserts the bytecode instructions into the returned top-level function. Returns `NULL` if a compile-error occurred.
ObjFunction *compile(const char *source);

// Marks all used dynamically allocated memory from the compiler.
void markCompilerRoots();

#endif
