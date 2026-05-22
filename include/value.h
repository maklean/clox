#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

// Constant pool
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

// Creates a `Value` corresponding to the given C value.
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj *)object}})

// Gets the literal value from the given `Value`.
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

// Returns whether the given `Value` matches the specified variant.
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

// Initializes a value array.
void initValueArray(ValueArray *array);

// Writes to the given value array.
void writeValueArray(ValueArray* array, Value value);

// Frees the value array's data from memory.
void freeValueArray(ValueArray* array);

// Prints the given value.
void printValue(Value value);

// Returns whether the values are equivalent.
bool valuesEqual(Value a, Value b);

#endif