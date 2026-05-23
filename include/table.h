#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct {
    ObjString *key;
    Value value;
} Entry;

// Hash table
typedef struct {
  int count;
  int capacity;
  Entry *entries;
} Table;

// Initializes the hash table.
void initTable(Table *table);

// Frees the hashtable's entries from memory.
void freeTable(Table *table);

// Gets the value with the given key from the table. Returns whether the key-value pair was found.
bool tableGet(Table *table, ObjString *key, Value *value);

// Adds/Overwrites a key-value pair in the hash table. Returns true if an insertion happened, false if an overwrite happened.
bool tableSet(Table *table, ObjString *key, Value value);

// Deletes the entry with the given key from the hash table.
bool tableDelete(Table *table, ObjString *key);

// Copies all entries from the 'from' table to the 'to' table.
void tableAddAll(Table *from, Table *to);

// Attempts to find the 
ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash);

#endif