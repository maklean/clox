#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

// Constant pool
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

// Initializes a value array.
void initValueArray(ValueArray *array);

// Writes to the given value array.
void writeValueArray(ValueArray* array, Value value);

// Frees the value array's data from memory.
void freeValueArray(ValueArray* array);

// Prints the given value.
void printValue(Value value);

#endif