#include "../include/value.h"
#include "../include/memory.h"
#include "../include/object.h"

#include <stdio.h>
#include <string.h>

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
#ifdef NAN_BOXING
    if(IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if(IS_NIL(value)) {
        printf("nil");
    } else if(IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if(IS_OBJ(value)) {
        printObject(value);
    }
#else
    switch(value.type) {
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
        case VAL_NIL: printf("nil"); break;
        case VAL_BOOL: printf("%s", AS_BOOL(value) ? "true" : "false"); break;
        case VAL_OBJ: printObject(value); break;
    }
#endif
}

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
    // for IEEE 754 NaN != NaN compliance
    if(IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    
    return a == b;
#else
    if(a.type != b.type) return false;

    // toy language and it's already better than JS' type coercion.
    switch(a.type) {
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL: return true;
        case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);

        default: return false; // Unreachable.
    }
#endif
}