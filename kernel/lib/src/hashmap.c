//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "../include/hashmap.h"
#include "../include/string.h"
#include "../../kalloc/kalloc.h"
#include "../../lib/include/memset.h"
#include "../../lib/include/panic.h"
#include "../../lib/include/types.h"

// Life cycle management

int hashmap_init(struct hashmap *map, size_t bucket_count, hash_func_t hash_func, key_cmp_func_t key_cmp, key_free_func_t key_free)
{
    if (!map || bucket_count == 0 || !hash_func || !key_cmp)
    {
        return -1;
    }

    // Allocate bucket array
    map->buckets = kzalloc(bucket_count * sizeof(struct list));
    if (!map->buckets)
    {
        return -1;
    }

    // Initialize all buckets
    for (size_t i = 0; i < bucket_count; i++)
    {
        lst_init(&map->buckets[i]);
    }

    map->bucket_count = bucket_count;
    map->size = 0;
    map->hash_func = hash_func;
    map->key_cmp = key_cmp;
    map->key_free = key_free;

    return 0;
}

void hashmap_destroy(struct hashmap *map)
{
    if (!map)
    {
        return;
    }

    // Clear all entries
    hashmap_clear(map);

    // Free bucket array
    if (map->buckets)
    {
        kfree(map->buckets);
        map->buckets = NULL;
    }

    map->bucket_count = 0;
    map->size = 0;
    map->hash_func = NULL;
    map->key_cmp = NULL;
    map->key_free = NULL;
}

// Main operations

int hashmap_insert(struct hashmap *map, void *key, void *value)
{
    if (!map || !key)
    {
        return -1;
    }

    // Calculate bucket index
    uint64_t hash = map->hash_func(key);
    size_t bucket_idx = hash % map->bucket_count;

    // Search for existing entry with same key
    struct list *bucket = &map->buckets[bucket_idx];
    struct list *node;
    for (node = bucket->next; node != bucket; node = node->next)
    {
        struct hashmap_entry *entry = container_of(node, struct hashmap_entry, list_node);
        if (map->key_cmp(entry->key, key) == 0)
        {
            // Update existing entry
            entry->value = value;
            return 0;
        }
    }

    // Create new entry
    struct hashmap_entry *entry = kzalloc(sizeof(struct hashmap_entry));
    if (!entry)
    {
        return -1;
    }

    entry->key = key;
    entry->value = value;
    lst_init(&entry->list_node);

    // Add to bucket
    lst_push(bucket, &entry->list_node);
    map->size++;

    return 0;
}

void *hashmap_get(struct hashmap *map, const void *key)
{
    if (!map || !key)
    {
        return NULL;
    }

    // Calculate bucket index
    uint64_t hash = map->hash_func(key);
    size_t bucket_idx = hash % map->bucket_count;

    // Search in bucket
    struct list *bucket = &map->buckets[bucket_idx];
    struct list *node;
    for (node = bucket->next; node != bucket; node = node->next)
    {
        struct hashmap_entry *entry = container_of(node, struct hashmap_entry, list_node);
        if (map->key_cmp(entry->key, key) == 0)
        {
            return entry->value;
        }
    }

    return NULL;
}

int hashmap_remove(struct hashmap *map, const void *key)
{
    if (!map || !key)
    {
        return -1;
    }

    // Calculate bucket index
    uint64_t hash = map->hash_func(key);
    size_t bucket_idx = hash % map->bucket_count;

    // Search in bucket
    struct list *bucket = &map->buckets[bucket_idx];
    struct list *node;
    for (node = bucket->next; node != bucket; node = node->next)
    {
        struct hashmap_entry *entry = container_of(node, struct hashmap_entry, list_node);
        if (map->key_cmp(entry->key, key) == 0)
        {
            // Remove from list
            lst_remove(&entry->list_node);

            // Free key if callback is set
            if (map->key_free)
            {
                map->key_free(entry->key);
            }

            kfree(entry);
            map->size--;
            return 0;
        }
    }

    return -1;
}

void hashmap_clear(struct hashmap *map)
{
    if (!map || !map->buckets)
    {
        return;
    }

    // Clear all buckets
    for (size_t i = 0; i < map->bucket_count; i++)
    {
        struct list *bucket = &map->buckets[i];
        while (!lst_empty(bucket))
        {
            struct list *node = (struct list *) lst_pop(bucket);
            struct hashmap_entry *entry = container_of(node, struct hashmap_entry, list_node);

            // Free key if callback is set
            if (map->key_free)
            {
                map->key_free(entry->key);
            }

            kfree(entry);
        }
    }

    map->size = 0;
}

// Utility functions

size_t hashmap_size(const struct hashmap *map)
{
    if (!map)
    {
        return 0;
    }
    return map->size;
}

int hashmap_is_empty(const struct hashmap *map)
{
    if (!map)
    {
        return 1;
    }
    return map->size == 0;
}

// Hash functions

// Uses djb2 hashing algorithm
uint64_t hashmap_hash_string(const void *key)
{
    const char *str = (const char *) key;
    uint64_t hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

uint64_t hashmap_hash_ptr(const void *key)
{
    uintptr_t ptr = (uintptr_t) key;
    return hashmap_hash_uint64(ptr);
}

uint64_t hashmap_hash_uint64(const void *key)
{
    uint64_t x = (uint64_t) key;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Comparison functions

int hashmap_cmp_string(const void *key1, const void *key2)
{
    const char *str1 = (const char *) key1;
    const char *str2 = (const char *) key2;
    return strcmp(str1, str2);
}

int hashmap_cmp_ptr(const void *key1, const void *key2)
{
    const void *ptr1 = (const void *) key1;
    const void *ptr2 = (const void *) key2;
    if (ptr1 < ptr2)
        return -1;
    if (ptr1 > ptr2)
        return 1;
    return 0;
}

int hashmap_cmp_uint64(const void *key1, const void *key2)
{
    const uint64_t *val1 = (const uint64_t *) key1;
    const uint64_t *val2 = (const uint64_t *) key2;
    if (*val1 < *val2)
        return -1;
    if (*val1 > *val2)
        return 1;
    return 0;
}
