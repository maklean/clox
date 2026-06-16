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

// Returns whether the type of the value is a bound method.
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

// Returns whether the type of the value is an array.
#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)

// Returns whether the type of the value is a type method.
#define IS_TYPE_METHOD(value) isObjType(value, OBJ_TYPE_METHOD)

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

// Returns the value as an `ObjBoundMethod *`
#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))

// Returns the value as an `ObjArray *`
#define AS_ARRAY(value) ((ObjArray *)AS_OBJ(value))

// Returns the function pointed to by an `ObjTypeMethod *`.
#define AS_TYPE_METHOD(value) (((ObjTypeMethod *)AS_OBJ(value))->function)

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
    OBJ_NATIVE,
    OBJ_UPVALUE,
    OBJ_CLOSURE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
    OBJ_ARRAY,
    OBJ_TYPE_METHOD,
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
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure *method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    ValueArray data;
} ObjArray;

typedef Value (*NativeFn)(int argCount, Value *args);
typedef bool (*TypeMethod)(int argCount, Value *args, Value *result);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

typedef struct {
    Obj obj;
    TypeMethod function;
} ObjTypeMethod;

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

// Heap-allocates a new `ObjBoundMethod` struct.
ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);

// Heap-allocates a new `ObjArray` struct.
ObjArray *newArray();

// Heap-allocates a new `ObjTypeMethod` struct.
ObjTypeMethod *newTypeMethod(TypeMethod function);

// Allocates a heap-allocated `ObjString` made from 'chars' and 'length'.
ObjString *takeString(char *chars, int length);

// Copies 'length' characters starting at 'chars' and returns a heap-allocated `ObjString`.
ObjString *copyString(const char *chars, int length);

// Prints the object value.
void printObject(Value value);

// Returns whether the type of a value matches the given type.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif