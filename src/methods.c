#include "../include/methods.h"
#include "../include/table.h"
#include "../include/vm.h"

#include <string.h>

extern VM vm;

// Defines the given type method on the given hash table.
static void defineTypeMethod(Table *table, const char *name, TypeMethod fnc);

// Returns the length of the array.
static bool array_len(int argCount, Value *args, Value *result);

void initMethods(Table *arrMethods, Table *strMethods) {
    defineTypeMethod(arrMethods, "len", array_len);
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
    if(argCount != 0) {
        runtimeError("Hell nah.");
        return false;
    }

    *result = NUMBER_VAL((AS_ARRAY(args[0]))->data.count);
    
    return true;
}