#include "../include/methods.h"
#include "../include/table.h"
#include "../include/vm.h"

#include <string.h>
#include <math.h>

extern VM vm;

// Checks if the argument count matches the expected count. `argCount` and `expected` should be literals with no side effects.
#define CHECK_ARGUMENT_COUNT(argCount, expected, method) \
    do { \
        if(argCount != expected) { \
            runtimeError("%s expected %d argument(s), received %d.", method, expected, argCount); \
            return false; \
        } \
    } while(0)

// Checks for a whole number index. `val_index` should be a literal with no side effects.
#define CHECK_VALID_INDEX(val_index, method) \
    do { \
        if(floor(val_index) != val_index) { \
            runtimeError("Index passed to %s has to be a whole number.", method); \
            return false; \
        } \
    } while(0)

// Checks for out-of-bounds access into the given array.
#define CHECK_OUT_OF_BOUNDS(index, n, method) \
    do { \
        if(index < 0 || index >= n) { \
            runtimeError("Index passed to %s: %d, is out-of-bounds.", method, index); \
            return false; \
        } \
    } while(0)
            
// Defines the given type method on the given hash table.
static void defineTypeMethod(Table *table, const char *name, TypeMethod fnc);

// Array Methods
static bool array_len(int argCount, Value *args, Value *result);
static bool array_pop(int argCount, Value *args, Value *result);
static bool array_push(int argCount, Value *args, Value *result);
static bool array_get(int argCount, Value *args, Value *result);
static bool array_set(int argCount, Value *args, Value *result);
static bool array_insert(int argCount, Value *args, Value *result);
static bool array_clear(int argCount, Value *args, Value *result);
static bool array_isEmpty(int argCount, Value *args, Value *result);
static bool array_copy(int argCount, Value *args, Value *result);

void initMethods(Table *arrMethods, Table *strMethods) {
    defineTypeMethod(arrMethods, "len", array_len);
    defineTypeMethod(arrMethods, "pop", array_pop);
    defineTypeMethod(arrMethods, "push", array_push);
    defineTypeMethod(arrMethods, "get", array_get);
    defineTypeMethod(arrMethods, "set", array_set);
    defineTypeMethod(arrMethods, "insert", array_insert);
    defineTypeMethod(arrMethods, "clear", array_clear);
    defineTypeMethod(arrMethods, "isEmpty", array_isEmpty);
    defineTypeMethod(arrMethods, "copy", array_copy);
}

static void defineTypeMethod(Table *table, const char *name, TypeMethod fnc) {
    // add to VM stack so the GC doesn't clean up early
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newTypeMethod(fnc)));

    tableSet(table, AS_STRING(vm.stack[0]), vm.stack[1]);

    pop();
    pop();
}

static bool array_len(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "arr.len()");

    *result = NUMBER_VAL((AS_ARRAY(args[0]))->data.count);

    return true;
}

static bool array_pop(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "arr.pop()");

    ObjArray *arr = AS_ARRAY(args[0]);

    if(arr->data.count == 0) {
        runtimeError("cannot arr.pop() on empty array.");
        return false;
    }

    *result = arr->data.values[--arr->data.count];
    return true;
}

static bool array_push(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.push(val)");

    ObjArray *arr = AS_ARRAY(args[0]);
    Value value = args[1];

    push(value); // prevent GC bugs
    writeValueArray(&arr->data, value);
    pop();

    return true;
}

static bool array_get(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.get(i)");

    ObjArray *arr = AS_ARRAY(args[0]);
    double val_index = AS_NUMBER(args[1]);

    CHECK_VALID_INDEX(val_index, "arr.get(i)");

    int index = (int)val_index;
    int n = arr->data.count;

    CHECK_OUT_OF_BOUNDS(index, n, "arr.get(i)");

    *result = arr->data.values[index];
    return true;
}

static bool array_set(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 2, "arr.set(i, val)");

    ObjArray *arr = AS_ARRAY(args[0]);
    double val_index = AS_NUMBER(args[1]);

    CHECK_VALID_INDEX(val_index, "arr.set(i, val)");

    int index = (int)val_index;
    int n = arr->data.count;

    CHECK_OUT_OF_BOUNDS(index, n, "arr.set(i, val)");

    arr->data.values[index] = args[2];
    return true;
}

static bool array_insert(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 2, "arr.insert(i, val)");

    ObjArray *arr = AS_ARRAY(args[0]);
    double val_index = AS_NUMBER(args[1]);

    CHECK_VALID_INDEX(val_index, "arr.insert(i, val)");

    int index = (int)val_index;
    int n = arr->data.count;

    // perform push() if adding at the array length
    if(index == n) {
        writeValueArray(&arr->data, args[2]);
        return true;
    }

    CHECK_OUT_OF_BOUNDS(index, n, "arr.insert(i, val)");

    insertValueArray(&arr->data, args[2], index);
    return true;
}

static bool array_clear(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "arr.clear()");

    ObjArray *arr = AS_ARRAY(args[0]);
    arr->data.count = 0;

    return true;
}

static bool array_isEmpty(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "arr.isEmpty()");

    ObjArray *arr = AS_ARRAY(args[0]);

    *result = BOOL_VAL(arr->data.count == 0);
    return true;
}

static bool array_copy(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "arr.copy()");

    ObjArray *arr = AS_ARRAY(args[0]);
    ObjArray *copy_arr = newArray();

    push(OBJ_VAL(copy_arr)); // prevent GC bugs

    // only a shallow copy
    for(int i = 0; i < arr->data.count; i++) {
        writeValueArray(&copy_arr->data, arr->data.values[i]);
    }

    pop();

    *result = OBJ_VAL(copy_arr);
    return true;
}