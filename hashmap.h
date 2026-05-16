#ifndef HASHMAP
#define HASHMAP

typedef struct _Hash_Map Hash_Map;

const char *hash_map_get(const char *key);
const char *hash_map_set(const char *key, const char *value);

void _set_hash_map();
void _free_hash_map();

#endif
