#include "../include/methods.h"
#include "../include/table.h"
#include "../include/vm.h"

#include <string.h>
#include <math.h>

extern VM vm;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
static bool array_remove(int argCount, Value *args, Value *result);
static bool array_contains(int argCount, Value *args, Value *result);
static bool array_indexOf(int argCount, Value *args, Value *result);
static bool array_slice(int argCount, Value *args, Value *result);
static bool array_concat(int argCount, Value *args, Value *result);
static bool array_reverse(int argCount, Value *args, Value *result);
static bool array_join(int argCount, Value *args, Value *result);
static bool array_clear(int argCount, Value *args, Value *result);
static bool array_isEmpty(int argCount, Value *args, Value *result);
static bool array_copy(int argCount, Value *args, Value *result);

// String Methods
static bool string_len(int argCount, Value *args, Value *result);

void initMethods(Table *arrMethods, Table *strMethods) {
    defineTypeMethod(arrMethods, "len", array_len);
    defineTypeMethod(arrMethods, "pop", array_pop);
    defineTypeMethod(arrMethods, "push", array_push);
    defineTypeMethod(arrMethods, "get", array_get);
    defineTypeMethod(arrMethods, "set", array_set);
    defineTypeMethod(arrMethods, "insert", array_insert);
    defineTypeMethod(arrMethods, "remove", array_remove);
    defineTypeMethod(arrMethods, "contains", array_contains);
    defineTypeMethod(arrMethods, "indexOf", array_indexOf);
    defineTypeMethod(arrMethods, "slice", array_slice);
    defineTypeMethod(arrMethods, "concat", array_concat);
    defineTypeMethod(arrMethods, "reverse", array_reverse);
    defineTypeMethod(arrMethods, "join", array_join);
    defineTypeMethod(arrMethods, "clear", array_clear);
    defineTypeMethod(arrMethods, "isEmpty", array_isEmpty);
    defineTypeMethod(arrMethods, "copy", array_copy);

    defineTypeMethod(strMethods, "len", string_len);
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

    // 'value' is already on the stack, so no need for push-pop to prevent the GC from accidentally cleaning it up
    writeValueArray(&arr->data, value);

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

static bool array_remove(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.remove(i)");

    ObjArray *arr = AS_ARRAY(args[0]);
    double val_index = AS_NUMBER(args[1]);

    CHECK_VALID_INDEX(val_index, "arr.remove(i)");

    int index = (int)val_index;
    int n = arr->data.count;

    CHECK_OUT_OF_BOUNDS(index, n, "arr.remove(i)");

    removeValueArray(&arr->data, index);
    return true;
}

static bool array_contains(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.contains(val)");

    ObjArray *arr = AS_ARRAY(args[0]);
    Value value = args[1];

    for(int i = 0; i < arr->data.count; i++) {
        if(valuesEqual(value, arr->data.values[i])) {
            *result = TRUE_VAL;
            return true;
        }
    }

    *result = FALSE_VAL;
    return true;
}

static bool array_indexOf(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.indexOf(val)");

    ObjArray *arr = AS_ARRAY(args[0]);
    Value value = args[1];

    for(int i = 0; i < arr->data.count; i++) {
        if(valuesEqual(value, arr->data.values[i])) {
            *result = NUMBER_VAL(i);
            return true;
        }
    }

    *result = NUMBER_VAL(-1);
    return true;
}

static bool array_slice(int argCount, Value *args, Value *result) {
    if(argCount == 0) {
        CHECK_ARGUMENT_COUNT(argCount, 1, "arr.slice(a, b=n)");
    } else if(argCount > 2) {
        CHECK_ARGUMENT_COUNT(argCount, 2, "arr.slice(a, b=n)");
    }

    ObjArray *arr = AS_ARRAY(args[0]);
    int n = arr->data.count;

    double val_index_a = AS_NUMBER(args[1]);
    double val_index_b = argCount == 1 ? n : AS_NUMBER(args[2]);

    CHECK_VALID_INDEX(val_index_a, "arr.slice(a, b=n)");
    CHECK_VALID_INDEX(val_index_b, "arr.slice(a, b=n)");

    // these should be clamped if they're out-of-bounds towards the right
    int index_a = MIN((int)val_index_a, n);
    int index_b = MIN((int)val_index_b, n);

    // check for negative indexes
    CHECK_OUT_OF_BOUNDS(index_a, n+1, "arr.slice(a, b=n)");
    CHECK_OUT_OF_BOUNDS(index_b, n+1, "arr.slice(a, b=n)");
    
    if(index_b < index_a) {
        runtimeError("index b cannot be smaller than index a in arr.slice(a, b=n).");
        return false;
    }

    // atp, everything should be in the correct state to start copying: index_a <= index_b, index_a & index_b are in [0, n]
    ObjArray *slicedArray = newArray();

    push(OBJ_VAL(slicedArray)); // prevent GC bugs

    for(int i = index_a; i < index_b; i++) {
        writeValueArray(&slicedArray->data, arr->data.values[i]);
    }

    pop();

    *result = OBJ_VAL(slicedArray);
    return true;
}

static bool array_concat(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.concat(other)");

    ObjArray *arr = AS_ARRAY(args[0]);
    Value other_val = args[1];

    if(!IS_ARRAY(other_val)) {
        runtimeError("Can only concatenate with another array in array.concat(other).");
        return false;
    }

    ObjArray *other = AS_ARRAY(other_val);
    int n = other->data.count;

    for(int i = 0; i < n; i++) {
        writeValueArray(&arr->data, other->data.values[i]);
    }

    return true;
}

static bool array_reverse(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "arr.reverse()");

    ObjArray *arr = AS_ARRAY(args[0]);

    // +150 LeetCode problems solved have prepared me for this exact moment...
    int l = 0;
    int r = arr->data.count-1;

    Value tmp;
    while(l < r) {
        tmp = arr->data.values[l];
        arr->data.values[l] = arr->data.values[r];
        arr->data.values[r] = tmp;

        l++;
        r--;
    }

    // Clean solution, O(n) time complexity, O(1) space, still no FAANG offer.

    return true;
}

static bool array_join(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 1, "arr.join(sep)");

    ObjArray *arr = AS_ARRAY(args[0]);
    Value sep_val = args[1];

    if(!IS_STRING(sep_val)) {
        runtimeError("Seperator must be a string in arr.join(sep).");
        return false;
    }

    int n = arr->data.count;

    // ensure n >= 1
    if(n == 0) {
        *result = OBJ_VAL(copyString("", 0));
        return true;
    }

    size_t arr_char_count = 0;
    for(int i = 0; i < n; i++) {
        // check for any non-string values in the array - basically what Python does
        if(!IS_STRING(arr->data.values[i])) {
            runtimeError("Expected string at index %d when joining array.", i);
            return false;
        }

        arr_char_count += (AS_STRING(arr->data.values[i]))->length;
    }

    ObjString *sep = AS_STRING(sep_val);
    size_t sep_n = sep->length;

    size_t result_str_n = arr_char_count + sep_n * (n-1);
    char result_str[result_str_n];
    
    char *ptr = result_str; // cursor into result_str
    ObjString *s;

    for(int i = 0; i < n; i++) {
        s = AS_STRING(arr->data.values[i]);

        memcpy(ptr, s->chars, s->length); // copy chars from 's' where 'ptr'
        ptr += (uintptr_t)s->length; // move to where seperator has to go

        // add seperator if we're between elements
        if(i != n-1) {
            memcpy(ptr, sep->chars, sep_n);
            ptr += (uintptr_t)sep_n;
        }
    }

    *result = OBJ_VAL(copyString(result_str, result_str_n));

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

static bool string_len(int argCount, Value *args, Value *result) {
    CHECK_ARGUMENT_COUNT(argCount, 0, "str.len()");

    ObjString *str = AS_STRING(args[0]);
    
    *result = NUMBER_VAL(str->length);
    return true;
}