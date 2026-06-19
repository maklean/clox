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

// Prints the elements of the given array.
static void printArray(ObjArray *array);

ObjFunction *newFunction() {
    ObjFunction *function = (ObjFunction *)ALLOCATE_OBJ(sizeof(ObjFunction), OBJ_FUNCTION);

    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);

    return function;
}

ObjNative *newNative(NativeFn function) {
    ObjNative *native = (ObjNative *)ALLOCATE_OBJ(sizeof(ObjNative), OBJ_NATIVE);

    native->function = function;

    return native;
}

ObjClosure *newClosure(ObjFunction *function) {
    // allocate array of upvalues for closure
    ObjUpvalue **upvalues = ALLOCATE(sizeof(ObjUpvalue *), function->upvalueCount);
    for(int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = (ObjClosure *)ALLOCATE_OBJ(sizeof(ObjClosure), OBJ_CLOSURE);
    
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;

    return closure;
}

ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = (ObjUpvalue *)ALLOCATE_OBJ(sizeof(ObjUpvalue), OBJ_UPVALUE);

    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;

    return upvalue;
}

ObjClass *newClass(ObjString *name) {
    ObjClass *klass = (ObjClass *)ALLOCATE_OBJ(sizeof(ObjClass), OBJ_CLASS);

    klass->name = name;
    initTable(&klass->methods);
    #ifdef INLINE_CACHING
    initTable(&klass->fieldNames);
    #endif

    return klass;
}

ObjInstance *newInstance(ObjClass *klass) {
    ObjInstance *instance = (ObjInstance *)ALLOCATE_OBJ(sizeof(ObjInstance), OBJ_INSTANCE);

    instance->klass = klass;
    #ifdef INLINE_CACHING
    initValueArray(&instance->fields);
    #else
    initTable(&instance->fields);
    #endif

    return instance;
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    ObjBoundMethod *bound = (ObjBoundMethod *)ALLOCATE_OBJ(sizeof(ObjBoundMethod), OBJ_BOUND_METHOD);

    bound->receiver = receiver;
    bound->method = method;

    return bound;
}

ObjArray *newArray() {
    ObjArray *arr = (ObjArray *)ALLOCATE_OBJ(sizeof(ObjArray), OBJ_ARRAY);

    initValueArray(&arr->data);

    return arr;
}

ObjTypeMethod *newTypeMethod(TypeMethod function) {
    ObjTypeMethod *method = (ObjTypeMethod *)ALLOCATE_OBJ(sizeof(ObjTypeMethod), OBJ_TYPE_METHOD);

    method->function = function;

    return method;
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
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_ARRAY:
            printArray(AS_ARRAY(value));
            break;
        case OBJ_TYPE_METHOD:
            printf("<type method fn>");
            break;
    }
}

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *)reallocate(NULL, 0, size);

    object->type = type;
    object->next = vm.objects;
    object->isMarked = false;
    
    vm.objects = object;

    #ifdef DEBUG_LOG_GC
        printf("%p allocate %zu for %d\n", (void*)object, size, type);
    #endif

    return object;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash) {
    ObjString *string = (ObjString *)ALLOCATE_OBJ(sizeof(ObjString), OBJ_STRING);

    string->chars = chars;
    string->length = length;
    string->hash = hash;

    // add to VM stack so it doesn't get cleaned up by the GC if the hash table has to resize.
    push(OBJ_VAL(string));

    tableSet(&vm.strings, string, NIL_VAL);

    pop();

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

static void printArray(ObjArray *array) {
    int n = array->data.count;

    printf("[");

    Value v;
    for(int i = 0; i < n; i++) {
        v = array->data.values[i];

        if(IS_STRING(v)) printf("\"");
        printValue(array->data.values[i]);
        if(IS_STRING(v)) printf("\"");

        if(i != n-1) printf(", ");
    }

    printf("]");
}