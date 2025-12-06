//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef TMPFS_H
#define TMPFS_H

#include "../../vfs/vfs.h"

// Source: /usr/include/linux/magic.h
#define TMPFS_MAGIC 0x01021994

// Private data for tmpfs inode
struct tmpfs_inode_info {
    void *data;              // For files: pointer to data
    struct list entries;     // For directories: list of entries
    uint64_t data_size;      // Size of allocated memory
};

// Directory entry in tmpfs
struct tmpfs_dir_entry {
    char name[MAX_NAME_LEN];
    struct inode *inode;
    struct list list_node;   // For entries list
};

/**
 * tmpfs filesystem info
 * @note Stub for now
 */
struct tmpfs_fs_info {
    uint64_t blocks_used;
    uint64_t blocks_total;
};

// tmpfs initialization
int tmpfs_init(void);
struct superblock* tmpfs_mount(void);
int tmpfs_unmount(struct superblock *sb);

// tmpfs operations
extern struct inode_operations tmpfs_file_inode_ops;
extern struct file_operations tmpfs_file_ops;
extern struct inode_operations tmpfs_dir_inode_ops;
extern struct file_operations tmpfs_dir_file_ops;
extern struct superblock_operations tmpfs_sb_ops;

#endif // TMPFS_H

