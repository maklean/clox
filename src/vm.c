#include "../include/common.h"
#include "../include/debug.h"
#include "../include/vm.h"
#include "../include/compiler.h"
#include "../include/value.h"
#include "../include/memory.h"
#include "../include/methods.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>

VM vm;

// Native function that returns the current elapsed time since the program started running.
static Value clockNative(int argCount, Value *args);

static InterpretResult run();
static void resetStack();

// Peeks `distance` away from the top of the stack.
static Value peek(int distance);

// Returns whether the value is falsey or not (is nil or false)
static bool isFalsey(Value value);

// Concatenates the top two strings on the stack.
static void concatenate();

// Creates a new frame on the CallFrame stack, returns whether the operation was successful.
static bool callValue(Value callee, int argCount);

// Wraps the given value in an ObjUpvalue.
static ObjUpvalue *captureUpvalue(Value *local);

// Moves a local variable from the stack onto the heap.
static void closeUpvalues(Value *last);

// Calls the given function, returns whether the call was successful.
static bool call(ObjClosure *closure, int argCount);

// Defines a class method using the method closure at the top of the stack.
static void defineMethod(ObjString *name);

// Pushes an ObjBoundMethod bound to the most recent object instance onto the stack.
static bool bindMethod(ObjClass* klass, ObjString* name);

// Calls the specific invoke method on the value `argCount` away from the top of the stack.
static bool invoke(ObjString *name, int argCount);

// Invokes the given method on the given instance.
static bool invokeInstance(ObjString *name, int argCount, ObjInstance *instance);

// Invokes the given method on the given type method object `argCount` away from the top of the stack.
static bool invokeTypeMethod(ObjString *name, int argCount, const char *type, Table *typeTable);

// Calls the given method from the given class object. Returns whether the call was successful or not.
static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount);

// Creates a native function
static void defineNative(const char *name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);

    // pop both off the stack so the GC doesn't clean it up
    pop();
    pop();
}

void initVM() {
    resetStack();

    vm.initString = NULL; // since the GC can activate before actually setting the string
    vm.objects = NULL;
    vm.grayCount = vm.grayCapacity = 0;
    vm.grayStack = NULL;

    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    initTable(&vm.strings);
    initTable(&vm.globals);
    initTable(&vm.arrayMethods);
    initTable(&vm.stringMethods);

    vm.initString = copyString("init", 4);

    defineNative("clock", clockNative);
    initMethods(&vm.arrayMethods, &vm.stringMethods);
}

void freeVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeTable(&vm.arrayMethods);
    freeTable(&vm.stringMethods);
    vm.initString = NULL;
    freeObjects();
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount-1]; // use the top-most CallFrame (currently executing function)

    #define READ_BYTE() (*frame->ip++)
    #define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
    #define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
    #define READ_CONSTANT_LONG() (frame->closure->function->chunk.constants.values[(READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE()])
    #define READ_STRING() AS_STRING(READ_CONSTANT())
    #define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())
    #define BINARY_OP(valueType, op) \
        do { \
            if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
                runtimeError("Expected two number operands at the top of the stack."); \
                return INTERPRET_RUNTIME_ERROR; \
            } \
            double b = AS_NUMBER(pop()); \
            double a = AS_NUMBER(pop()); \
            push(valueType(a op b)); \
        } while(0)
    #define BINARY_OP_LIBFUNC(valueType, func) \
        do { \
            if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
                runtimeError("Expected two number operands at the top of the stack."); \
                return INTERPRET_RUNTIME_ERROR; \
            } \
            double b = AS_NUMBER(pop()); \
            double a = AS_NUMBER(pop()); \
            push(valueType(func(a, b))); \
        } while(0)

    for(;;) {
        #ifdef DEBUG_TRACE_EXECUTION
            // print stack
            printf("          ");

            for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }

            printf("\n");

            disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
        #endif

        uint8_t instruction;

        switch(instruction = READ_BYTE()) {
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                vm.frameCount--;

                // it was the top-level function CallFrame we just removed, the program is done.
                if(vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                // go back to the previous CallFrame with the returned (popped) value
                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_CONSTANT:
            case OP_CONSTANT_LONG: {
                Value constant = instruction == OP_CONSTANT ? READ_CONSTANT() : READ_CONSTANT_LONG();
                push(constant);
                break;
            }

            case OP_ADD: {
                if(IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    BINARY_OP(NUMBER_VAL, +);
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_POW: BINARY_OP_LIBFUNC(NUMBER_VAL, pow); break;
            case OP_MOD: BINARY_OP_LIBFUNC(NUMBER_VAL, fmod); break;

            case OP_NEGATE:
                // Check if the value at the top of the stack is a number.
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            
            case OP_TRUE: case OP_FALSE: push(BOOL_VAL(instruction == OP_TRUE)); break;
            case OP_NIL: push(NIL_VAL); break;

            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            
            case OP_PRINT:
                Value expr = pop();
                printValue(expr);
                printf("\n");
                break;
            
            case OP_POP: pop(); break;

            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                ObjString *name = instruction == OP_DEFINE_GLOBAL ? READ_STRING() : READ_STRING_LONG();
                
                // set global init. value as the item at the top of the stack
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            
            case OP_GET_GLOBAL:
            case OP_GET_GLOBAL_LONG: {
                ObjString *name = instruction == OP_GET_GLOBAL ? READ_STRING() : READ_STRING_LONG();
                Value value;

                // get global value
                if(!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(value);
                break;
            }
            case OP_SET_GLOBAL:
            case OP_SET_GLOBAL_LONG: {
                ObjString *name = instruction == OP_SET_GLOBAL ? READ_STRING() : READ_STRING_LONG();

                if(tableSet(&vm.globals, name, peek(0))) {
                    // if true is returned, an insertion happened, so the user has peformed assignment on an undefined variable.
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                break;
            }

            case OP_GET_LOCAL:
            case OP_GET_LOCAL_LONG: {
                int slot = instruction == OP_GET_LOCAL ? READ_BYTE() : ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE());
                push(frame->slots[slot]); // the slot should match the slot in the locals array
                break;
            }
            
            case OP_SET_LOCAL:
            case OP_SET_LOCAL_LONG: {
                int slot = instruction == OP_SET_LOCAL ? READ_BYTE() : ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE());
                frame->slots[slot] = peek(0); // overwrite the value at the slot with the new value
                break;
            }

            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                
                // skip then-branch if falsey
                if(isFalsey(peek(0))) frame->ip += offset;
                break;
            }

            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset; // go backwards
                break;
            }

            case OP_CALL: {
                int argCount = READ_BYTE();
                if(!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_CLOSURE:
            case OP_CLOSURE_LONG: {
                // get function from constant table
                ObjFunction *function = AS_FUNCTION(instruction == OP_CLOSURE ? READ_CONSTANT() : READ_CONSTANT_LONG());

                // wrap in closure
                ObjClosure *closure = newClosure(function);
                push(OBJ_VAL(closure));

                for(int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    bool isLongIndex = isLocal & 2;
                    isLocal &= 1;

                    int index;
                    if(isLongIndex) {
                        int b1 = READ_BYTE();
                        int b2 = READ_BYTE();
                        int b3 = READ_BYTE();
                        index = (b1 << 16) | (b2 << 8) | b3;
                    } else {
                        index = READ_BYTE();
                    }

                    if(isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }

            case OP_GET_UPVALUE: 
            case OP_GET_UPVALUE_LONG: {
                int slot = instruction == OP_GET_UPVALUE ? READ_BYTE() : ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE());
                push(*frame->closure->upvalues[slot]->location);
                break;
            }

            case OP_SET_UPVALUE:
            case OP_SET_UPVALUE_LONG: {
                int slot = instruction == OP_SET_UPVALUE ? READ_BYTE() : ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE());
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }

            case OP_CLOSE_UPVALUE:
                // variable-to-be-hoisted should be at the top of the stack
                closeUpvalues(vm.stackTop-1);
                pop();
                break;
            
            case OP_CLASS:
            case OP_CLASS_LONG:
                push(OBJ_VAL(newClass(instruction == OP_CLASS ? (READ_STRING()) : (READ_STRING_LONG()))));
                break;
            
            case OP_GET_PROPERTY:
            case OP_GET_PROPERTY_LONG: {
                // property access should only be valid on an instance
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = instruction == OP_GET_PROPERTY ? (READ_STRING()) : (READ_STRING_LONG());

                Value value;
                #ifdef INLINE_CACHING
                    if(tableGet(&instance->klass->fieldNames, name, &value)) {
                        // remove instance and add field value to stack
                        pop();
                        int index = (int)AS_NUMBER(value);
                        push(instance->fields.values[index]);
                        break;
                    }
                #else
                    if(tableGet(&instance->fields, name, &value)) {
                        // remove instance and add field value to stack
                        pop();
                        push(value);
                        break;
                    }
                #endif

                if(!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SET_PROPERTY:
            case OP_SET_PROPERTY_LONG: {
                // property access should only be valid on an instance
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(1));
                
                #ifdef INLINE_CACHING
                    ObjString *name = instruction == OP_SET_PROPERTY ? (READ_STRING()) : (READ_STRING_LONG());
                    Value index_val;
                    int index;
                    
                    if(tableGet(&instance->klass->fieldNames, name, &index_val)) {
                        // we're assigning to a property we've already stored in the fields array
                        index = (int)AS_NUMBER(index_val);

                        if(instance->fields.count <= index) {
                            writeValueArray(&instance->fields, peek(0));
                        } else {
                            instance->fields.values[index] = peek(0);
                        }
                    } else {
                        // we're assigning to a property that isn't on the instance yet (this breaks IC, but since this is a learning project, it is what it is...)
                        index = instance->fields.count;
                        
                        writeValueArray(&instance->fields, peek(0));
                        tableSet(&instance->klass->fieldNames, name, NUMBER_VAL(index));
                    }
                #else
                    tableSet(&instance->fields, (instruction == OP_SET_PROPERTY ? (READ_STRING()) : (READ_STRING_LONG())), peek(0));
                #endif
                
                // get value, pop instance, add value back to the stack (since it should be the result of the expression)
                Value value = pop();
                pop();
                push(value);

                break;
            }

            case OP_METHOD:
            case OP_METHOD_LONG:
                defineMethod(instruction == OP_METHOD ? (READ_STRING()) : (READ_STRING_LONG()));
                break;
            
            case OP_INVOKE:
            case OP_INVOKE_LONG: {
                ObjString *method = instruction == OP_INVOKE ? (READ_STRING()) : (READ_STRING_LONG());
                int argCount = READ_BYTE();

                // call the method with the given name (instance obj should be `argCount` away from the top of the stack atp)
                if(!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                // point to new frame on VM
                frame = &vm.frames[vm.frameCount - 1];

                break;
            }

            case OP_INHERIT: {
                if(!IS_CLASS(peek(1))) {
                    runtimeError("Superclass must be a class");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjClass *superclass = AS_CLASS(peek(1));
                ObjClass *subclass = AS_CLASS(peek(0));

                // add all superclass methods to the subclass methods
                tableAddAll(&superclass->methods, &subclass->methods);
                
                pop(); // rmv. subclass from the stack

                break;
            }

            case OP_GET_SUPER:
            case OP_GET_SUPER_LONG: {
                ObjString *name = instruction == OP_GET_SUPER ? (READ_STRING()) : (READ_STRING_LONG());
                ObjClass *superclass = AS_CLASS(pop());

                if(!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }

            case OP_SUPER_INVOKE:
            case OP_SUPER_INVOKE_LONG: {
                ObjString *method = instruction == OP_SUPER_INVOKE ? (READ_STRING()) : (READ_STRING_LONG());
                int argCount = READ_BYTE();

                ObjClass *superclass = AS_CLASS(pop());

                // try to call method from super class directly
                if(!invokeFromClass(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                // point to new frame on VM
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_ARRAY: {
                int n = (int)READ_BYTE();

                ObjArray *arr = newArray();
                push(OBJ_VAL(arr)); // array will resize while adding elements so we need to add it so the GC doesn't clean it

                // there should be n values on the stack (they're in reverse tho)
                for(int i = n; i >= 1; i--) {
                    writeValueArray(&arr->data, peek(i));
                }

                vm.stackTop -= n + 1; // remove/pop all n values off the stack + array object

                push(OBJ_VAL(arr));

                break;
            }

            case OP_INDEX: {
                // the array's object and the index should be sitting on the stack
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Index has to be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if(!IS_ARRAY(peek(1)) && !IS_STRING(peek(1))) {
                    runtimeError("Can only get index into an array or string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                double val_index = AS_NUMBER(pop());
                if(floor(val_index) != val_index) {
                    runtimeError("Index has to be a whole number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int index = (int)val_index;

                if(IS_ARRAY(peek(0))) {
                    ObjArray *arr = (ObjArray *)AS_ARRAY(pop());

                    int n = arr->data.count;

                    // check for out-of-bounds access
                    if(index < 0 || index >= n) {
                        runtimeError("Attempted out-of-bounds access into array.");
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    push(arr->data.values[index]);
                } else {
                    ObjString *str = (ObjString *)AS_STRING(pop());

                    int n = str->length;

                    // check for out-of-bounds access
                    if(index < 0 || index >= n) {
                        runtimeError("Attempted out-of-bounds access into array.");
                        return INTERPRET_RUNTIME_ERROR;
                    }

                    push(OBJ_VAL(copyString(str->chars+index, 1)));
                }
                
                break;
            };

            case OP_SET_INDEX: {
                // value should be at the top of the stack, the index should follow, last should be the array object
                if(!IS_NUMBER(peek(1))) {
                    runtimeError("Index has to be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                if(!IS_ARRAY(peek(2))) {
                    runtimeError("Can only set index into an array.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = pop();
                
                double val_index = AS_NUMBER(pop());
                if(floor(val_index) != val_index) {
                    runtimeError("Index has to be a whole number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int index = (int)val_index;

                ObjArray *arr = (ObjArray *)AS_ARRAY(pop());

                int n = arr->data.count;

                if(index < 0 || index >= n) {
                    runtimeError("Attempted out-of-bounds access into array.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                arr->data.values[index] = value;
                
                // since it's an assignment, its result should be the value
                push(value);
                break;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_SHORT
    #undef READ_CONSTANT
    #undef READ_CONSTANT_LONG
    #undef READ_STRING
    #undef READ_STRING_LONG
    #undef BINARY_OP
    #undef BINARY_OP_LIBFUNC
}

InterpretResult interpret(const char *source) {
    ObjFunction *function = compile(source);
    
    // there's been a compile-time error
    if(function == NULL) return INTERPRET_COMPILE_ERROR;

    // push top-level function as the first value
    push(OBJ_VAL(function));
    
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-distance - 1]; // -1 b/c stackTop is positioned where the next value goes.
}

void runtimeError(const char *format, ...) {
    // print the formatted string
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // find instruction index in each CallFrame, and print error message
    for(int i = vm.frameCount-1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;

        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);

        if(function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && AS_BOOL(value) == false);
}

static void concatenate() {
    // peek() the strings so they don't mistakingly get unmarked if the GC triggers when creating the new string object
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char *chars = ALLOCATE(sizeof(char), length+1);

    // copy characters from a, b, then add null-terminator
    memcpy(chars, a->chars, a->length);
    memcpy(chars+a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);

    // string was creating successfully, clean up the two strings
    pop();
    pop();

    push(OBJ_VAL(result));
}

static bool callValue(Value callee, int argCount) {
    if(IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
            case OBJ_CLASS: {
                ObjClass *klass = AS_CLASS(callee);

                // replace class object on the stack with new instance
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

                Value initializer;
                if(tableGet(&klass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);
                } else if(argCount != 0) {
                    // passed in arguments after no initializer was found
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }

                return true;
            }
            case OBJ_NATIVE:
                NativeFn native = AS_NATIVE(callee);

                // execute native function then move back to position before the arguments
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;

                push(result);
                return true;
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);

                // set instance at empty variable slot
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            default:
                break;
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = NULL, *upvalue = vm.openUpvalues;

    // try to find existing upvalue for local
    while(upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    // found existing upvalue, use that instead of creating new one
    if(upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    // add upvalue to openUpvalues list
    if(prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value *last) {
    // we use address comparison to only deal with variables that are in the list.
    while(vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;

        // move the value onto the heap by copying it into closed
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed; // this is so OP_GET/SET_UPVALUE can continue working with whatever's at *upvalue->location

        vm.openUpvalues = upvalue->next;
    }
}

static bool call(ObjClosure* closure, int argCount) {
    if(argCount != closure->function->arity) {
        runtimeError("Expected %d arguments, got %d.", closure->function->arity, argCount);
        return false;
    }

    if(vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];

    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1; // make it point the function statement on the stack

    return true;
}

static Value clockNative(int argCount, Value *args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void defineMethod(ObjString *name) {
    // use ObjClosure at the top of the stack
    Value method = peek(0);
    ObjClass *klass = AS_CLASS(peek(1));

    // store method in class, then remove it from the top of the stack
    tableSet(&klass->methods, name, method);

    pop();
}

static bool bindMethod(ObjClass* klass, ObjString* name) {
    // find method in class' method table
    Value method;
    if(!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    // create new bound method instance at the top of the stack
    ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));

    pop(); // rmv. instance from top of stack
    push(OBJ_VAL(bound));

    return true;
}

static bool invoke(ObjString *name, int argCount) {
    Value receiver = peek(argCount);

    if(IS_INSTANCE(receiver)) {
        return invokeInstance(name, argCount, AS_INSTANCE(receiver));
    } else if(IS_ARRAY(receiver) || IS_STRING(receiver)) {
        const char *type = IS_ARRAY(receiver) ? "array" : "string";
        Table *typeTable = IS_ARRAY(receiver) ? &vm.arrayMethods : &vm.stringMethods;

        return invokeTypeMethod(name, argCount, type, typeTable);
    }

    runtimeError("Cannot invoke method on this value.");
    return false;
}

static bool invokeInstance(ObjString *name, int argCount, ObjInstance *instance) {
    // look for field with same name before looking for function
    Value value;

    #ifdef INLINE_CACHING
    if(tableGet(&instance->klass->fieldNames, name, &value)) {
        int index = (int)AS_NUMBER(value);
        vm.stackTop[-argCount - 1] = instance->fields.values[index];

        return callValue(instance->fields.values[index], argCount);
    }
    #else
    if(tableGet(&instance->fields, name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }
    #endif

    // call method on instance
    return invokeFromClass(instance->klass, name, argCount);
}

static bool invokeTypeMethod(ObjString *name, int argCount, const char *type, Table *typeTable) {
    Value fncVal;

    if(!tableGet(typeTable, name, &fncVal)) {
        runtimeError("Undefined method '%s' on %s.", name->chars, type);
        return false;
    }

    TypeMethod methodFn = AS_TYPE_METHOD(fncVal);
    Value result = NIL_VAL;

    Value *args = vm.stackTop - argCount - 1; // points where the type method object is, followed by the arguments passed by the user

    bool ok = methodFn(argCount, args, &result);
    vm.stackTop -= argCount + 1; // remove arguments + type method object
    if(ok) push(result);

    return ok;
}

static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount) {
    Value method;

    if(!tableGet(&klass->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    return call(AS_CLOSURE(method), argCount);
}