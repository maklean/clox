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

// Returns the value as an `ObjString *`
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))

// Returns the characters in an `ObjString` value.
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

// Returns the value as an `ObjFunction *`
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING
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

// Heap-allocates a new `ObjFunction` struct.
ObjFunction *newFunction();

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