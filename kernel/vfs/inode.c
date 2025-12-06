//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements functions for managing inode structures.
//

#include "../kalloc/kalloc.h"
#include "../lib/include/memset.h"
#include "../lib/include/panic.h"
#include "vfs.h"

// Global inode counter
static uint64_t next_ino = 1;
static struct spinlock ino_lock;

void inode_init(void) {
    init_spinlock(&ino_lock, "ino_lock");
}

struct inode *vfs_alloc_inode(struct superblock *sb) {
    struct inode *inode = kalloc();
    if (!inode) {
        return NULL;
    }

    memset(inode, 0, sizeof(struct inode));

    acquire_spinlock(&ino_lock);
    inode->ino = next_ino++;
    release_spinlock(&ino_lock);

    inode->sb = sb;
    inode->ref = 1;
    inode->nlink = 1;
    init_spinlock(&inode->lock, "inode");

    return inode;
}

void vfs_free_inode(struct inode *inode) {
    if (!inode) return;

    if (inode->ref != 0) {
        panic("vfs_free_inode: ref != 0");
    }

    kfree(inode);
}

/**
 * Acquires an inode reference by incrementing the reference count.
 **/
struct inode *vfs_get_inode(struct inode *inode) {
    if (!inode) return NULL;

    acquire_spinlock(&inode->lock);
    inode->ref++;
    release_spinlock(&inode->lock);

    return inode;
}

/**
 * Releases an inode reference by decrementing the reference count.
 * @note If the reference count reaches zero, the inode is destroyed and freed.
 **/
void vfs_put_inode(struct inode *inode) {
    if (!inode) return;

    acquire_spinlock(&inode->lock);
    inode->ref--;
    uint32_t ref = inode->ref;
    release_spinlock(&inode->lock);

    if (ref == 0) {
        // Call destroy_inode from superblock if available
        if (inode->sb && inode->sb->s_op && inode->sb->s_op->destroy_inode) {
            inode->sb->s_op->destroy_inode(inode);
        }
        vfs_free_inode(inode);
    }
}
