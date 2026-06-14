#include "../include/memory.h"
#include "../include/object.h"
#include "../include/table.h"
#include "../include/value.h"

#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

// Returns a pointer to an empty (or existing) entry in the table for the given key.
static Entry *findEntry(Entry *entries, int capacity, ObjString *key);

// Allocates space for the entries in the hash table.
static void adjustCapacity(Table *table, int capacity);

void initTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table) {
    FREE_ARRAY(sizeof(Entry), table->entries, table->capacity);
    initTable(table);
}

bool tableGet(Table *table, ObjString *key, Value *value) {
    if(table->count == 0) return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if(entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

bool tableSet(Table *table, ObjString *key, Value value) {
    // grow table if the load factor is going to be reached after this insertion
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    
    // only increase count on insertion in an empty entry
    if(isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;

    return isNewKey;
}

bool tableDelete(Table *table, ObjString *key) {
    if(table->count == 0) return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if(entry->key == NULL) return false;

    // Delete value by placing tombstone
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

void tableAddAll(Table *from, Table *to) {
    for(int i = 0; i < from->capacity; i++) {
        Entry *fromEntry = &from->entries[i];
        
        if(fromEntry->key != NULL) {
            tableSet(to, fromEntry->key, fromEntry->value);
        }
    }
}

ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash) {
    if(table->count == 0) return NULL;

    uint32_t index = hash & (table->capacity - 1);
    
    for(;;) {
        Entry *entry = &table->entries[index];

        if(entry->key == NULL) {
            // found an empty entry, the string doesn't exist
            if(IS_NIL(entry->value)) return NULL;
        } else if(entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
            // found interned string
            return entry->key;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

void tableRemoveWhite(Table *table) {
    for(int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];

        // remove the string from the table if it's unmarked
        if(entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(Table *table) {
    for(int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];

        markObject((Obj *)entry->key);
        markValue(entry->value);
    }
}

static Entry *findEntry(Entry *entries, int capacity, ObjString *key) {
    uint32_t index = key->hash & (capacity - 1);
    Entry *tombstone = NULL;

    for(;;) {
        Entry *entry = &entries[index];

        if(entry->key == NULL) {
            if(IS_NIL(entry->value)) {
                // empty entry, return tombstone, if any
                return tombstone != NULL ? tombstone : entry;
            } else {
                // found a tombstone
                if(tombstone == NULL) tombstone = entry;
            }
        } else if(entry->key == key) {
            // found the key
            return entry;
        }

        // probe to another index
        index = (index + 1) & (capacity - 1);
    }

    // unreachable
    return NULL;
}

static void adjustCapacity(Table *table, int capacity) {
    Entry *entries = ALLOCATE(sizeof(Entry), capacity);
    
    // Set all entries to NULL pairs (empty bucket)
    for(int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;

    // re-insert everything from old table
    for(int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if(entry->key == NULL) continue;

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;

        table->count++;
    }

    FREE_ARRAY(sizeof(Entry), table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}