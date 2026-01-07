//
// Created by ShipOS developers on 06.01.26.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements functions for managing directories.
//

#include "../kalloc/kalloc.h"
#include "vfs.h"

int vfs_opendir(const char *path, struct file **result)
{
    if (!path || !result)
    {
        return VFS_EINVAL;
    }

    // Lookup path
    struct dentry *dentry = vfs_path_lookup(path);
    if (!dentry)
    {
        return VFS_ENOENT;
    }

    struct inode *inode = dentry->inode;
    if (!inode)
    {
        vfs_put_dentry(dentry);
        return VFS_EINVAL;
    }

    // Check if it's actually a directory
    if (inode->type != INODE_TYPE_DIR)
    {
        vfs_put_dentry(dentry);
        return VFS_ENOTDIR;
    }

    // Allocate file structure
    struct file *file = vfs_alloc_file();
    if (!file)
    {
        vfs_put_dentry(dentry);
        return VFS_ENOMEM;
    }

    file->inode = inode;
    file->dentry = dentry;
    file->flags = O_RDONLY;
    file->offset = 0;
    file->f_op = inode->f_op;

    vfs_get_inode(inode);

    // Call open operation if available
    if (file->f_op && file->f_op->open)
    {
        int ret = file->f_op->open(inode, file);
        if (ret != VFS_OK)
        {
            vfs_close(file);
            return ret;
        }
    }

    *result = file;
    return VFS_OK;
}

int vfs_readdir(struct file *file, struct dirent *dirent, uint64_t count)
{
    if (!file || !dirent || count == 0)
    {
        return VFS_EINVAL;
    }

    struct inode *inode = file->inode;
    if (!inode || inode->type != INODE_TYPE_DIR)
    {
        return VFS_ENOTDIR;
    }

    // Check if readdir operation is available
    if (!file->f_op || !file->f_op->readdir)
    {
        return VFS_EINVAL;
    }

    return file->f_op->readdir(file, dirent, count);
}
