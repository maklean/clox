#include "../include/memory.h"
#include "../include/object.h"
#include "../include/value.h"
#include "../include/vm.h"
#include "../include/table.h"

#include <stdio.h>
#include <string.h>

extern VM vm;

// Allocates a generic `Obj` with a size of `size` and type of `type` on the heap.
#define ALLOCATE_OBJ(typeSize, objType) allocateObject(typeSize, objType)

// Returns a heap-allocated generic `Obj` with a size of 'size' created from `type`.
static Obj *allocateObject(size_t size, ObjType type);

// Returns a heap-allocated `ObjString` created from 'chars' and 'length'.
static ObjString *allocateString(char *chars, int length, uint32_t hash);

// FNV-1a hashing function for strings.
static uint32_t hashString(const char *key, int length);

// Prints the given function to the console.
static void printFunction(ObjFunction *function);

ObjFunction *newFunction() {
    ObjFunction *function = (ObjFunction *)ALLOCATE_OBJ(sizeof(ObjFunction), OBJ_FUNCTION);

    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);

    return function;
}

ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);

    // use interned string if it exists
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if(interned != NULL) {
        FREE_ARRAY(sizeof(char), chars, length+1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);

    // use interned string if it exists
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if(interned != NULL) return interned;

    char *heapChars = (char *)ALLOCATE(sizeof(char), length+1);

    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length, hash);
}

void printObject(Value value) {
    switch(OBJ_TYPE(value)) {
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;

    return object;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash) {
    ObjString *string = (ObjString *)ALLOCATE_OBJ(sizeof(ObjString), OBJ_STRING);

    string->chars = chars;
    string->length = length;
    string->hash = hash;
    
    tableSet(&vm.strings, string, NIL_VAL);

    return string;
}

static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;

    for(int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

static void printFunction(ObjFunction *function) {
    if(function->name == NULL) {
        printf("<script>");
        return;
    }
    
    printf("<fn %s>", function->name->chars);
}