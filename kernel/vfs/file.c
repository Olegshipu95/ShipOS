//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements functions for managing file structures.
//

#include "../kalloc/kalloc.h"
#include "../lib/include/memset.h"
#include "../lib/include/panic.h"
#include "../lib/include/string.h"
#include "vfs.h"

struct file *vfs_alloc_file(void) {
    struct file *file = kalloc();
    if (!file) {
        return NULL;
    }

    memset(file, 0, sizeof(struct file));
    file->ref = 1;
    init_spinlock(&file->lock, "file");

    return file;
}

void vfs_free_file(struct file *file) {
    if (!file) return;

    if (file->ref != 0) {
        panic("vfs_free_file: ref != 0");
    }

    kfree(file);
}

/**
 * Opens a file at the specified path.
 * @note That this function accepts `struct file **result` as a parameter in order to return pointer to `struct file` through it
 * @note It is implented so, because we want caller to recive exit code of the operation
 **/
int vfs_open(const char *path, int flags, struct file **result) {
    if (!path || !result) {
        return VFS_EINVAL;
    }

    // Lookup path
    struct dentry *dentry = vfs_path_lookup(path);

    // If not found and O_CREAT is set, create the file
    if (!dentry && (flags & O_CREAT)) {
        struct dentry *root_dentry = vfs_get_root();

        // Find parent directory
        char path_copy[MAX_PATH_LEN];
        strncpy(path_copy, path, MAX_PATH_LEN - 1);
        path_copy[MAX_PATH_LEN - 1] = '\0';

        // Find last slash
        char *last_slash = NULL;
        for (char *p = path_copy; *p; p++) {
            if (*p == '/') {
                last_slash = p;
            }
        }

        struct dentry *parent;
        const char *filename;

        if (last_slash) {
            *last_slash = '\0';        // NOTE: at this points path_copy will turn from "/home/user/file.txt" to "/home/user"
            filename = last_slash + 1; // NOTE: filename will have value "file.txt"
            parent = vfs_path_lookup(path_copy[0] ? path_copy : "/");
        } else {
            // Fallback to root directory
            parent = root_dentry;
            vfs_get_dentry(parent);
            filename = path;
        }

        if (!parent) {
            return VFS_ENOENT;
        }

        // Create file
        struct inode *parent_inode = parent->inode;
        if (!parent_inode || parent_inode->type != INODE_TYPE_DIR) {
            vfs_put_dentry(parent);
            return VFS_ENOTDIR;
        }

        // Check if parent inode has create operation
        if (!parent_inode->i_op || !parent_inode->i_op->create) {
            vfs_put_dentry(parent);
            return VFS_EINVAL;
        }

        struct inode *new_inode = NULL;
        int ret = parent_inode->i_op->create(parent_inode, filename, &new_inode);
        if (ret != VFS_OK || !new_inode) {
            vfs_put_dentry(parent);
            return ret;
        }

        // Create dentry for new file
        dentry = vfs_alloc_dentry(filename, new_inode);
        if (!dentry) {
            vfs_put_inode(new_inode);
            vfs_put_dentry(parent);
            return VFS_ENOMEM;
        }

        dentry->parent = parent;
        acquire_spinlock(&parent->lock);
        // NOTE: insert head of sibling list to parent's children list
        lst_push(&parent->children, &dentry->sibling);
        release_spinlock(&parent->lock);

        vfs_put_dentry(parent);
    }

    if (!dentry) {
        return VFS_ENOENT;
    }

    struct inode *inode = dentry->inode;
    if (!inode) {
        vfs_put_dentry(dentry);
        return VFS_EINVAL;
    }

    // Check if it's a directory (can't open directories for reading/writing)
    if (inode->type == INODE_TYPE_DIR) {
        vfs_put_dentry(dentry);
        return VFS_EISDIR;
    }

    // Allocate file structure
    struct file *file = vfs_alloc_file();
    if (!file) {
        vfs_put_dentry(dentry);
        return VFS_ENOMEM;
    }

    file->inode = inode;
    file->dentry = dentry;
    file->flags = flags;
    file->offset = 0;
    file->f_op = inode->f_op;

    vfs_get_inode(inode);

    // Call open operation if available
    if (file->f_op && file->f_op->open) {
        int ret = file->f_op->open(inode, file);
        if (ret != VFS_OK) {
            vfs_close(file);
            return ret;
        }
    }

    // Handle O_TRUNC flag
    if (flags & O_TRUNC) {
        inode->size = 0;
    }

    *result = file;
    return VFS_OK;
}

int vfs_close(struct file *file) {
    if (!file) {
        return VFS_EINVAL;
    }

    // Call close operation if available
    if (file->f_op && file->f_op->close) {
        file->f_op->close(file);
    }

    // Release references
    if (file->inode) {
        vfs_put_inode(file->inode);
    }
    if (file->dentry) {
        vfs_put_dentry(file->dentry);
    }

    acquire_spinlock(&file->lock);
    file->ref--;
    uint32_t ref = file->ref;
    release_spinlock(&file->lock);

    if (ref == 0) {
        vfs_free_file(file);
    }

    return VFS_OK;
}
