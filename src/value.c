#include "../include/value.h"
#include "../include/memory.h"

#include <stdio.h>

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value) {
    // Grow value array if there's not enough space
    if(array->count >= array->capacity) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = (Value *)GROW_ARRAY(sizeof(Value), array->values, oldCapacity, array->capacity);
    }

    array->values[array->count++] = value;
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(sizeof(Value), array->values, array->capacity);
    
    // reset
    initValueArray(array);
}

void printValue(Value value) {
    switch(value.type) {
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
        case VAL_NIL: printf("nil"); break;
        case VAL_BOOL: printf("%s", AS_BOOL(value) ? "true" : "false");
    }
}

bool valuesEqual(Value a, Value b) {
    if(a.type != b.type) return false;

    // toy language and it's already better than JS' type coercion.
    switch(a.type) {
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL: return true;
        default: return false; // Unreachable.
    }
}