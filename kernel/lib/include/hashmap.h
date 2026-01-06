//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Generic hash map implementation.
// Supports arbitrary key and value types through void pointers.
// Uses linked lists (buckets) for collision resolution.
//

#ifndef HASHMAP_H
#define HASHMAP_H

#include <inttypes.h>
#include <stddef.h>
#include "../../list/list.h"

typedef uint64_t (*hash_func_t)(const void *key);
typedef int (*key_cmp_func_t)(const void *key1, const void *key2);
typedef void (*key_free_func_t)(void *key);

// Hash map entry
struct hashmap_entry
{
    void *key;             // Key
    void *value;           // Value
    struct list list_node; // For collision chain
};

// Hash map structure
struct hashmap
{
    struct list *buckets;     // Array of bucket lists
    size_t bucket_count;      // Number of buckets
    size_t size;              // Number of entries
    hash_func_t hash_func;    // Hash function
    key_cmp_func_t key_cmp;   // Key comparison function
    key_free_func_t key_free; // Key free function (optional)
};

int hashmap_init(struct hashmap *map, size_t bucket_count,
                 hash_func_t hash_func, key_cmp_func_t key_cmp,
                 key_free_func_t key_free);
void hashmap_destroy(struct hashmap *map);

int hashmap_insert(struct hashmap *map, void *key, void *value);
void *hashmap_get(struct hashmap *map, const void *key);
int hashmap_remove(struct hashmap *map, const void *key);
void hashmap_clear(struct hashmap *map);

size_t hashmap_size(const struct hashmap *map);
int hashmap_is_empty(const struct hashmap *map);

uint64_t hashmap_hash_string(const void *key);
uint64_t hashmap_hash_ptr(const void *key);
uint64_t hashmap_hash_uint64(const void *key);

int hashmap_cmp_string(const void *key1, const void *key2);
int hashmap_cmp_ptr(const void *key1, const void *key2);
int hashmap_cmp_uint64(const void *key1, const void *key2);

#endif // HASHMAP_H
