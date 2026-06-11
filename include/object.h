#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "table.h"

// Returns the type of the given value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Returns whether the type of the value is a string.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Returns whether the type of the value is a function.
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// Returns whether the type of the value is a closure.
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)

// Returns whether the type of the value is a class.
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)

// Returns whether the type of the value is a class instance.
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)

// Returns whether the type of the value is a native function.
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

// Returns the value as an `ObjString *`
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))

// Returns the characters in an `ObjString` value.
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

// Returns the value as an `ObjFunction *`
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))

// Returns the value as an `ObjClosure *`
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

// Returns the function pointed to by an `ObjNative *`.
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)

// Returns the value as an `ObjClass *`
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))

// Returns the value as an `ObjInstance *`
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
    OBJ_NATIVE,
    OBJ_UPVALUE,
    OBJ_CLOSURE,
    OBJ_CLASS,
    OBJ_INSTANCE,
} ObjType;

struct Obj {
  ObjType type;
  bool isMarked;
  struct Obj *next;
};

struct ObjString {
    Obj obj;
    int length;
    char *chars;
    uint32_t hash;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    int arity; // num. of params
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef struct {
    Obj obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int upvalueCount;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString *name;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *klass;
    Table fields;
} ObjInstance;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

// Heap-allocates a new `ObjFunction` struct.
ObjFunction *newFunction();

// Heap-allocates a new `ObjNative` struct.
ObjNative *newNative(NativeFn function);

// Heap-allocates a new `ObjClosure` struct.
ObjClosure *newClosure(ObjFunction *function);

// Heap-allocates a new `ObjUpvalue` struct.
ObjUpvalue *newUpvalue(Value *slot);

// Heap-allocates a new `ObjString` struct.
ObjClass *newClass(ObjString *name);

// Heap-allocates a new `ObjInstance` struct.
ObjInstance *newInstance(ObjClass *klass);

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