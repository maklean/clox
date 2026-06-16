#ifndef clox_methods_h
#define clox_methods_h

#include "value.h"
#include "object.h"

// Populates the array type methods and string type methods table.
void initMethods(Table *arrMethods, Table *strMethods);

#endif