#include "../include/memory.h"
#include "../include/object.h"
#include "../include/value.h"
#include "../include/vm.h"

#include <stdio.h>
#include <string.h>

extern VM vm;

// Allocates a generic `Obj` with a size of 'size' and type of `type` on the heap.
#define ALLOCATE_OBJ(typeSize, objType) allocateObject(typeSize, objType)

// Returns a heap-allocated generic `Obj` with a size of 'size' created from `type`.
static Obj *allocateObject(size_t size, ObjType type);

// Returns a heap-allocated `ObjString` created from 'chars' and 'length'.
static ObjString *allocateString(char *chars, int length);

ObjString *takeString(char *chars, int length) {
    return allocateString(chars, length);
}

ObjString *copyString(const char *chars, int length) {
    char *heapChars = (char *)ALLOCATE(sizeof(char), length+1);

    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length);
}

void printObject(Value value) {
    switch(OBJ_TYPE(value)) {
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

static ObjString *allocateString(char *chars, int length) {
    ObjString *string = (ObjString *)ALLOCATE_OBJ(sizeof(ObjString), OBJ_STRING);

    string->chars = chars;
    string->length = length;

    return string;
}

