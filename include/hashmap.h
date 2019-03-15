#ifndef _HASHMAP_H
#define _HASHMAP_H

#include <sys/uio.h>

#define BIT_MAPSIZE 12
#define HASHMAP_SIZE 4096

typedef struct iovec data_t;

typedef char bool_t;

typedef struct node_t
{
    data_t *key;
    data_t *value;
    struct node_t *next;
} node_t;

typedef struct hashmap_t
{
    size_t size;
    node_t *content[HASHMAP_SIZE];
} hashmap_t;

hashmap_t *init_map(void);
data_t *get_map(data_t *key, hashmap_t *map);
bool_t insert_map(data_t *key, data_t *value, hashmap_t *map);
bool_t contains_map(data_t *key, hashmap_t *map);
bool_t remove_map(data_t *key, hashmap_t *map);
size_t get_size_map(hashmap_t *map);
bool_t empty_map(hashmap_t *map);
void clear_map(hashmap_t *map);
void freehashmap(hashmap_t *map);
void print_hashmap(hashmap_t *map);

#endif