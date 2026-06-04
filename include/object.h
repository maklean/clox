#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"

// Returns the type of the given value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Returns whether the type of the value is a string.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Returns whether the type of the value is a function.
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// Returns whether the type of the value is a native function.
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

// Returns the value as an `ObjString *`
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))

// Returns the characters in an `ObjString` value.
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

// Returns the value as an `ObjFunction *`
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))

// Returns the function pointed to by an `ObjNative *`.
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
    OBJ_NATIVE,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj *next;
};

struct ObjString {
    Obj obj;
    int length;
    char *chars;
    uint32_t hash;
};

typedef struct {
    Obj obj;
    int arity; // num. of params
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

// Heap-allocates a new `ObjFunction` struct.
ObjFunction *newFunction();

// Heap-allocates a new `ObjNative` struct.
ObjNative *newNative(NativeFn function);

// Allocates a heap-allocated `ObjString` made from 'chars' and 'length'.
ObjString *takeString(char *chars, int length);

// Copies 'length' characters starting at 'chars' and returns a heap-allocated `ObjString`.
ObjString* copyString(const char* chars, int length);

// Prints the object value.
void printObject(Value value);

// Returns whether the type of a value matches the given type.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif