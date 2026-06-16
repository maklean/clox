#include "../include/memory.h"
#include "../include/object.h"
#include "../include/vm.h"
#include "../include/compiler.h"

#include <stdlib.h>

#ifdef DEBUG_LOG_GC
    #include "../include/debug.h"

    #include <stdio.h>
#endif

extern VM vm;

// Frees the given Object.
static void freeObject(Obj *object);

// Marks all roots (locals & temporaries on the VM stack, globals, compiler ObjFunction, etc.).
static void markRoots();

// Traverses through the stack of gray objects to color their references.
static void traceReferences();

// Cleans up all white (unused) objects from the heap.
static void sweep();

// Goes through all references from the given object and marks them, then it blackens the object.
static void blackenObject(Obj *object);

// Marks all the values in the given array.
static void markArray(ValueArray *array);

// Multiplier for next GC threshold
#define GC_HEAP_GROW_FACTOR 2

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;

    if(newSize > oldSize) {
        #ifdef DEBUG_STRESS_GC
            // trigger GC for every new allocated piece of memory if stress test is on
            collectGarbage();
        #endif

        if(vm.bytesAllocated > vm.nextGC) {
            // if we've reached the bytes allocated threshold, we should exit.
            collectGarbage();
        }
    }

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

    free(vm.grayStack);
}

void collectGarbage() {
    #ifdef DEBUG_LOG_GC
        printf("-- gc begin\n");
    #endif

    size_t before = vm.bytesAllocated;

    markRoots();
    traceReferences();
    sweep();

    // set next threshold.
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

    #ifdef DEBUG_LOG_GC
        printf("-- gc end\n");

        printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
            before - vm.bytesAllocated, before, vm.bytesAllocated,
            vm.nextGC);
    #endif
}

void markValue(Value value) {
    // we don't need to mark any integers, booleans, or nil since their data stored in the Value struct directly.
    if(IS_OBJ(value)) {
        markObject(AS_OBJ(value));
    }
}

void markObject(Obj *object) {
    if(object == NULL || object->isMarked) return;

    #ifdef DEBUG_LOG_GC
        printf("%p mark ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif

    object->isMarked = true;

    // resize grayStack if needed
    if(vm.grayCount == vm.grayCapacity) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj **)realloc(vm.grayStack, sizeof(Obj *) * vm.grayCapacity);
    }

    vm.grayStack[vm.grayCount++] = object;

    if(vm.grayStack == NULL) exit(1);
}

static void freeObject(Obj *object) {
    #ifdef DEBUG_LOG_GC
        printf("%p free type %d\n", (void*)object, object->type);
    #endif

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
        case OBJ_CLOSURE:
            ObjClosure *closure = (ObjClosure *)object;
            FREE_ARRAY(sizeof(ObjClosure *), closure->upvalues, closure->upvalueCount);
            FREE(sizeof(ObjClosure), object);
            break;
        case OBJ_UPVALUE:
            FREE(sizeof(ObjUpvalue), object);
            break;
        case OBJ_CLASS: {
            ObjClass *klass = (ObjClass *)object;
            freeTable(&klass->methods);
            FREE(sizeof(ObjClass), object);
            break;
        } 
        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance *)object;
            freeTable(&instance->fields);

            FREE(sizeof(ObjInstance), object);

            break;
        }
        case OBJ_BOUND_METHOD:
            FREE(sizeof(ObjBoundMethod), object);
            break;
        case OBJ_ARRAY:
            ObjArray *arr = (ObjArray *)object;
            freeValueArray(&arr->data);
            FREE(sizeof(ObjArray), object);
            break;
        case OBJ_TYPE_METHOD:
            FREE(sizeof(ObjTypeMethod), object);
            break;
    }
}

static void markRoots() {
    // Mark all locals & temporaries on the VM stack
    for(Value *slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    // mark globals and type methods
    markTable(&vm.globals);
    markTable(&vm.arrayMethods);
    markTable(&vm.stringMethods);

    // mark heap-allocated memory used by the compiler
    markCompilerRoots();

    // mark the closures that live on the CallFrame array
    for(int i = 0; i < vm.frameCount; i++) {
        markObject((Obj *)vm.frames[i].closure);
    }

    // mark open upvalues
    for(ObjUpvalue *upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj *)upvalue);
    }

    // mark init method string
    markObject((Obj *)vm.initString);
}

static void traceReferences() {
    while(vm.grayCount > 0) {
        Obj *object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep() {
    Obj *previous = NULL;
    Obj *object = vm.objects;

    while(object != NULL) {
        if(object->isMarked) {
            object->isMarked = false; // reset for next mark & sweep

            // if the object is marked, go to the next object in the list
            previous = object;
            object = object->next;
        } else {
            Obj *unreached = object;
            object = object->next;

            // if the object isn't marked, delete the node in the linked-list & free the object
            if(previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

static void blackenObject(Obj *object) {
    #ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif

    switch(object->type) {
        case OBJ_FUNCTION: {
            ObjFunction *function = (ObjFunction *)object;

            // mark function ObjString name and values in constant pool
            markObject((Obj *)function->name);
            markArray(&function->chunk.constants);

            break;
        }

        case OBJ_UPVALUE:
            markValue(((ObjUpvalue *)object)->closed);
            break;
        
        case OBJ_CLOSURE: {
            ObjClosure *closure = (ObjClosure *)object;
            
            // mark function wrapped by closure, and upvalues
            markObject((Obj *)closure->function);
            for(int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj *)closure->upvalues[i]);
            }

            break;
        }

        case OBJ_CLASS: {
            ObjClass *klass = (ObjClass *)object;
            markObject((Obj *)klass->name);
            markTable(&klass->methods);
            break;
        }

        case OBJ_INSTANCE: {
            ObjInstance *instance = (ObjInstance *)object;
            markObject((Obj *)instance->klass);
            markTable(&instance->fields);
            break;
        }

        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *bound = (ObjBoundMethod *)object;
            markValue(bound->receiver);
            markObject((Obj *)bound->method);
            break;
        }

        case OBJ_NATIVE:
        case OBJ_STRING:
        case OBJ_TYPE_METHOD:
            break;
        
        case OBJ_ARRAY: {
            ObjArray *arr = (ObjArray *)object;
            markArray(&arr->data);
            break;
        }
    }
}

static void markArray(ValueArray *array) {
    for(int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}