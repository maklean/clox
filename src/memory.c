#include "../include/memory.h"
#include "../include/object.h"
#include "../include/vm.h"

#include <stdlib.h>

extern VM vm;

static void freeObject(Obj *object);

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    if(newSize == 0) {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);
    if(result == NULL) exit(EXIT_FAILURE);

    return result;
}

void freeObjects() {
    Obj *object = vm.objects;
    Obj *next = NULL;

    while(object != NULL) {
        next = object->next;
        freeObject(object);
        object = next;    
    }
}

static void freeObject(Obj *object) {
    switch(object->type) {
        case OBJ_FUNCTION:
            ObjFunction *function = (ObjFunction *)object;
            freeChunk(&function->chunk);
            FREE(sizeof(ObjFunction), object);
            break;
        case OBJ_STRING:
            ObjString *str = (ObjString *)object;
            FREE_ARRAY(sizeof(char), str->chars, str->length+1);
            FREE(sizeof(ObjString), object);
            break;
        case OBJ_NATIVE:
            FREE(sizeof(ObjNative), object);
            break;
    }
}