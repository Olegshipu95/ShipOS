//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef TMPFS_H
#define TMPFS_H

#include "../../vfs/vfs.h"
#include "../../lib/include/hashmap.h"

// Source: /usr/include/linux/magic.h
#define TMPFS_MAGIC 0x01021994

#define TMPFS_DIR_BUCKETS 32

// Private data for tmpfs inode
struct tmpfs_inode_info
{
    void *data;               // For files: pointer to data
    uint64_t data_size;       // Size of allocated memory
    struct hashmap entries;   // For directories: hashmap of tmpfs_dir_entry (key: name string, value: tmpfs_dir_entry*)
    struct list entries_list; // For directories: list for readdir iteration (same entries as in hashmap)
};

// Directory entry in tmpfs
struct tmpfs_dir_entry
{
    char name[MAX_NAME_LEN];
    struct inode *inode;

    // It is an intrusive list
    // Allows to embed pointer to next node directly to the struct (hence, no need of external list)
    // Uses `container_of` to get the container structure - `tmpfs_dir_entry`.
    struct list list_node;
};

/**
 * tmpfs filesystem info
 * @note Stub for now
 */
struct tmpfs_fs_info
{
    uint64_t blocks_used;
    uint64_t blocks_total;
};

// tmpfs initialization
int tmpfs_init(void);
struct superblock *tmpfs_mount(void);
int tmpfs_unmount(struct superblock *sb);

// tmpfs operations
extern struct inode_operations tmpfs_file_inode_ops;
extern struct file_operations tmpfs_file_ops;
extern struct inode_operations tmpfs_dir_inode_ops;
extern struct file_operations tmpfs_dir_file_ops;
extern struct superblock_operations tmpfs_sb_ops;

#endif // TMPFS_H
