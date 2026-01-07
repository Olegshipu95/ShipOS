//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements functions for managing dentry structures.
//

#include "../kalloc/kalloc.h"
#include "../lib/include/memset.h"
#include "../lib/include/panic.h"
#include "../lib/include/string.h"
#include "vfs.h"

struct dentry *vfs_alloc_dentry(const char *name, struct inode *inode)
{
    if (!name || !inode)
    {
        return NULL;
    }

    struct dentry *dentry = kzalloc(sizeof(struct dentry));
    if (!dentry)
    {
        return NULL;
    }

    // Copy name
    size_t name_len = strlen(name);
    if (name_len >= MAX_NAME_LEN)
    {
        name_len = MAX_NAME_LEN - 1;
    }
    strncpy(dentry->name, name, name_len);
    dentry->name[name_len] = '\0';

    dentry->inode = inode;
    dentry->parent = NULL;
    dentry->ref = 1;

    lst_init(&dentry->children);
    lst_init(&dentry->sibling);
    init_spinlock(&dentry->lock, "dentry");

    // Acquire inode reference
    vfs_get_inode(inode);

    return dentry;
}

void vfs_free_dentry(struct dentry *dentry)
{
    if (!dentry)
        return;

    if (dentry->ref != 0)
    {
        panic("vfs_free_dentry: ref != 0");
    }

    // Release inode reference
    if (dentry->inode)
    {
        vfs_put_inode(dentry->inode);
    }

    kfree(dentry);
}

/**
 * Acquires a dentry reference by incrementing the reference count.
 **/
struct dentry *vfs_get_dentry(struct dentry *dentry)
{
    if (!dentry)
        return NULL;

    acquire_spinlock(&dentry->lock);
    dentry->ref++;
    release_spinlock(&dentry->lock);

    return dentry;
}

/**
 * Releases a dentry reference by decrementing the reference count.
 * @note If the reference count reaches zero, the dentry is freed.
 **/
void vfs_put_dentry(struct dentry *dentry)
{
    if (!dentry)
        return;

    acquire_spinlock(&dentry->lock);
    dentry->ref--;
    uint32_t ref = dentry->ref;
    release_spinlock(&dentry->lock);

    if (ref == 0)
    {
        vfs_free_dentry(dentry);
    }
}

/**
 * Looks up a child dentry in the parent directory.
 *
 * @example
 *   vfs_lookup(vfs_get_root(), "home")
 **/
struct dentry *vfs_lookup(struct dentry *parent, const char *name)
{
    if (!parent || !name)
    {
        return NULL;
    }

    struct inode *parent_inode = parent->inode;
    // Check if parent is a directory
    if (!parent_inode || parent_inode->type != INODE_TYPE_DIR)
        return NULL;
    // Check if operations are available
    if (!parent_inode->i_op || !parent_inode->i_op->lookup)
        return NULL;

    // Try to find in cache first
    if (dentry_cache_initialized)
    {
        struct dentry *cached = dentry_cache_lookup(parent_inode, name);
        if (cached)
        {
            return cached;
        }
    }

    // Use inode's lookup operation to find child
    struct inode *child_inode = parent_inode->i_op->lookup(parent_inode, name);
    if (!child_inode)
    {
        return NULL;
    }

    // Create dentry for found inode
    struct dentry *child_dentry = vfs_alloc_dentry(name, child_inode);
    if (!child_dentry)
    {
        vfs_put_inode(child_inode);
        return NULL;
    }

    // Set parent and add to parent's children list
    child_dentry->parent = parent;
    acquire_spinlock(&parent->lock);
    // NOTE: insert head of sibling list to parent's children list
    lst_push(&parent->children, &child_dentry->sibling);
    release_spinlock(&parent->lock);

    // Add to cache
    if (dentry_cache_initialized)
    {
        dentry_cache_add(child_dentry);
    }

    return child_dentry;
}
