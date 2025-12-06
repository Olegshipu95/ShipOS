//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements the global VFS functions.
//

#include "vfs.h"
#include "../lib/include/panic.h"
#include "../tty/tty.h"

// Forward declaration
extern struct superblock *tmpfs_mount(void);
extern void inode_init(void);

// Global root dentry
static struct dentry *root_dentry = NULL;
static struct superblock *root_sb = NULL;

struct dentry *vfs_get_root(void)
{
    return root_dentry;
}

int vfs_init(void)
{
    printf("Initializing VFS...\n");

    // Initialize inode subsystem
    inode_init();

    // Mount tmpfs as root filesystem
    root_sb = tmpfs_mount();
    if (!root_sb)
    {
        panic("Failed to mount root tmpfs");
        return VFS_ERR;
    }

    // Create root dentry
    root_dentry = vfs_alloc_dentry("/", root_sb->s_root);
    if (!root_dentry)
    {
        panic("Failed to create root dentry");
        return VFS_ERR;
    }

    printf("VFS initialized with tmpfs as root\n");
    return VFS_OK;
}
