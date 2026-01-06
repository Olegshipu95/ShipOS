#include "../lib/include/hashmap.h"
#include "../lib/include/types.h"
#include "../lib/include/string.h"
#include "../kalloc/kalloc.h"
#include "vfs.h"

#define DENTRY_CACHE_BUCKETS 256

struct dentry_cache_key
{
    struct inode *parent_inode;
    const char *name;
};

static struct hashmap dentry_cache;
static struct spinlock dentry_cache_lock;
int dentry_cache_initialized = 0;

static uint64_t dentry_cache_hash(const void *key)
{
    const struct dentry_cache_key *k = (const struct dentry_cache_key *) key;
    uint64_t hash = hashmap_hash_ptr((void *) k->parent_inode);
    // Combine with name hash
    if (k->name)
    {
        uint64_t name_hash = hashmap_hash_string(k->name);
        hash = hash ^ (name_hash << 1);
    }
    return hash;
}

static int dentry_cache_cmp(const void *key1, const void *key2)
{
    const struct dentry_cache_key *k1 = (const struct dentry_cache_key *) key1;
    const struct dentry_cache_key *k2 = (const struct dentry_cache_key *) key2;

    if (k1->parent_inode != k2->parent_inode)
    {
        return 1;
    }

    if (!k1->name || !k2->name)
    {
        return 1;
    }

    return strcmp(k1->name, k2->name);
}

int dentry_cache_init(void)
{
    if (dentry_cache_initialized)
    {
        return 0;
    }

    init_spinlock(&dentry_cache_lock, "dentry_cache");

    if (hashmap_init(&dentry_cache, DENTRY_CACHE_BUCKETS, dentry_cache_hash, dentry_cache_cmp, (key_free_func_t) kfree) != 0)
    {
        return -1;
    }

    dentry_cache_initialized = 1;
    return 0;
}

void dentry_cache_destroy(void)
{
    if (!dentry_cache_initialized)
    {
        return;
    }

    hashmap_destroy(&dentry_cache);
    dentry_cache_initialized = 0;
}

struct dentry *dentry_cache_lookup(struct inode *parent, const char *name)
{
    if (!dentry_cache_initialized || !parent || !name)
    {
        return NULL;
    }

    struct dentry_cache_key key = {
        .parent_inode = parent,
        .name = name};

    acquire_spinlock(&dentry_cache_lock);
    struct dentry *dentry = (struct dentry *) hashmap_get(&dentry_cache, &key);
    if (dentry)
    {
        vfs_get_dentry(dentry);
    }
    release_spinlock(&dentry_cache_lock);

    return dentry;
}

void dentry_cache_add(struct dentry *dentry)
{
    if (!dentry_cache_initialized || !dentry || !dentry->parent || !dentry->name)
    {
        return;
    }

    struct dentry_cache_key *key = kzalloc(sizeof(struct dentry_cache_key));
    if (!key)
    {
        return;
    }

    key->parent_inode = dentry->parent->inode;
    key->name = dentry->name;

    acquire_spinlock(&dentry_cache_lock);

    // Check if dentry is already in cache
    struct dentry *existing = (struct dentry *) hashmap_get(&dentry_cache, key);
    if (existing == dentry)
    {
        // Already in cache with same dentry - just free the key and return
        kfree(key);
        release_spinlock(&dentry_cache_lock);
        return;
    }

    if (existing)
    {
        // Different dentry with same key - remove old one first
        // This shouldn't happen in normal operation, but handle it gracefully
        hashmap_remove(&dentry_cache, key);
        vfs_put_dentry(existing);
    }

    vfs_get_dentry(dentry);

    hashmap_insert(&dentry_cache, key, dentry);
    release_spinlock(&dentry_cache_lock);
}

void dentry_cache_remove(struct dentry *dentry)
{
    if (!dentry_cache_initialized || !dentry || !dentry->parent || !dentry->name)
    {
        return;
    }

    // Create temporary key for lookup
    struct dentry_cache_key search_key = {
        .parent_inode = dentry->parent->inode,
        .name = dentry->name};

    acquire_spinlock(&dentry_cache_lock);
    // NOTE: hashmap_remove will automatically free the key using the callback
    int removed = hashmap_remove(&dentry_cache, &search_key);
    if (removed == 0)
    {
        vfs_put_dentry(dentry);
    }
    release_spinlock(&dentry_cache_lock);
}
