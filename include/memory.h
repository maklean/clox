#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// Double capacity if it's >= to 8, otherwise, default to 8.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

// Grows the array using reallocate()
#define GROW_ARRAY(typeSize, pointer, oldCount, newCount) \
    reallocate(pointer, (typeSize) * (oldCount), (typeSize) * (newCount))

// Frees the given array from memory using reallocate()
#define FREE_ARRAY(typeSize, pointer, oldCount) \
    reallocate(pointer, (typeSize) * (oldCount), 0)

// Reallocates memory for the given pointer from 'oldSize' to 'newSize'.
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif