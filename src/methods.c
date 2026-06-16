#include "../include/methods.h"
#include "../include/table.h"
#include "../include/vm.h"

#include <string.h>

extern VM vm;

// Checks if the argument count matches the expected count. `argCount` and `expected` should be literals with no side effects.
#define CHECK_ARGUMENT_COUNT(argCount, expected, method) \
    do { \
        if(argCount != expected) { \
            runtimeError("%s expected %d arguments, received %d.", method, expected, argCount); \
            return false; \
        } \
    } while(0)

// Defines the given type method on the given hash table.
static void defineTypeMethod(Table *table, const char *name, TypeMethod fnc);

// Array Methods
static bool array_len(int argCount, Value *args, Value *result);
static bool array_pop(int argCount, Value *args, Value *result);

void initMethods(Table *arrMethods, Table *strMethods) {
    defineTypeMethod(arrMethods, "len", array_len);
    defineTypeMethod(arrMethods, "pop", array_pop);
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