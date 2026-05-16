#include "hashmap.h"
#include <stdio.h>

#define hash_map_size 1000

typedef struct _Hash_Map_Entry _Hash_Map_Entry;

struct _Hash_Map_Entry{
    const char *key;
    const char *value;
    _Hash_Map_Entry *next;
};

struct _Hash_Map {
    _Hash_Map_Entry *cells[hash_map_size];   
};

Hash_Map HASH_MAP = {.cells = {NULL}};

int hashing_function(const char *key){
    
}
