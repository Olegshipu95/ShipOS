//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements functions that resolve paths.
//

#include "../lib/include/memset.h"
#include "../lib/include/string.h"
#include "vfs.h"

/**
 * Splits a path string into individual components.
 **/
int split_path(const char *path, char components[][MAX_NAME_LEN], int max_components)
{
    int count = 0;
    const char *start = path;

    // Skip leading slashes
    while (*start == '/')
    {
        start++;
    }

    while (*start && count < max_components)
    {
        const char *end = start;

        // Find next slash or end
        while (*end && *end != '/')
        {
            end++;
        }

        // Copy component
        int len = end - start;
        if (len > 0 && len < MAX_NAME_LEN)
        {
            memcpy(components[count], start, len);
            components[count][len] = '\0';
            count++;
        }

        // Move to next component
        start = end;
        while (*start == '/')
        {
            start++;
        }
    }

    return count;
}

/**
 * Resolves a full path to a dentry.
 *
 * @example
 *   vfs_path_lookup("/home/s1riys/dopsa")
 **/
struct dentry *vfs_path_lookup(const char *path)
{
    struct dentry *root_dentry = vfs_get_root();

    if (!path || !root_dentry)
    {
        return NULL;
    }

    // Handle root directory
    if (strcmp(path, "/") == 0)
    {
        vfs_get_dentry(root_dentry);
        return root_dentry;
    }

    // Split path into components
    char components[32][MAX_NAME_LEN];
    int num_components = split_path(path, components, 32);

    if (num_components == 0)
    {
        vfs_get_dentry(root_dentry);
        return root_dentry;
    }

    // Start from root
    struct dentry *current = root_dentry;
    vfs_get_dentry(current);

    // Walk through path components
    for (int i = 0; i < num_components; i++)
    {
        // Skip empty components
        if (components[i][0] == '\0')
        {
            continue;
        }

        // Handle "." (current directory)
        if (strcmp(components[i], ".") == 0)
        {
            continue;
        }

        // Handle ".." (parent directory)
        if (strcmp(components[i], "..") == 0)
        {
            if (current->parent)
            {
                struct dentry *parent = current->parent;
                vfs_get_dentry(parent);
                vfs_put_dentry(current);
                current = parent;
            }
            continue;
        }

        // Lookup child
        struct dentry *child = vfs_lookup(current, components[i]);
        if (!child)
        {
            vfs_put_dentry(current);
            return NULL;
        }

        vfs_put_dentry(current);
        current = child;

        // Check if the found dentry is a mount point - if so, switch to mounted filesystem root
        acquire_spinlock(&current->lock);
        struct superblock *mounted_sb = current->mounted_sb;
        release_spinlock(&current->lock);

        if (mounted_sb && mounted_sb->s_root)
        {
            struct dentry *mounted_root = NULL;

            // Try to find existing dentry for mounted root in cache
            if (dentry_cache_initialized)
            {
                mounted_root = dentry_cache_lookup(current->inode, "/");
            }

            // Create new dentry if not found in cache or invalid
            if (!mounted_root)
            {
                mounted_root = vfs_alloc_dentry("/", mounted_sb->s_root);
                if (!mounted_root)
                {
                    vfs_put_dentry(current);
                    return NULL;
                }
                mounted_root->parent = current;

                if (dentry_cache_initialized)
                {
                    dentry_cache_add(mounted_root);
                }
            }

            // Switch to mounted root
            vfs_put_dentry(current);
            current = mounted_root;

            continue;
        }
    }

    return current;
}
