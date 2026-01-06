//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements functions for reading and writing to files.
//

#include "vfs.h"

int64_t vfs_read(struct file *file, char *buf, uint64_t count)
{
    if (!file || !buf)
    {
        return VFS_EINVAL;
    }

    // Check if file is open for reading
    if ((file->flags & O_RDWR) == 0 && (file->flags & O_RDONLY) == 0)
    {
        return VFS_EINVAL;
    }

    // Check if read operation is available
    if (!file->f_op || !file->f_op->read)
    {
        return VFS_EINVAL;
    }

    return file->f_op->read(file, buf, count);
}

int64_t vfs_write(struct file *file, const char *buf, uint64_t count)
{
    if (!file || !buf)
    {
        return VFS_EINVAL;
    }

    // Check if file is open for writing
    if ((file->flags & O_RDWR) == 0 && (file->flags & O_WRONLY) == 0)
    {
        return VFS_EINVAL;
    }

    // Check if write operation is available
    if (!file->f_op || !file->f_op->write)
    {
        return VFS_EINVAL;
    }

    return file->f_op->write(file, buf, count);
}

int64_t vfs_lseek(struct file *file, int64_t offset, int whence)
{
    if (!file)
    {
        return VFS_EINVAL;
    }

    // If lseek operation is available, use it
    if (file->f_op && file->f_op->lseek)
    {
        return file->f_op->lseek(file, offset, whence);
    }

    // Default implementation
    acquire_spinlock(&file->lock);

    int64_t new_offset;
    switch (whence)
    {
    case SEEK_SET:
        new_offset = offset;
        break;
    case SEEK_CUR:
        new_offset = file->offset + offset;
        break;
    case SEEK_END:
        new_offset = file->inode->size + offset;
        break;
    default:
        release_spinlock(&file->lock);
        return VFS_EINVAL;
    }

    if (new_offset < 0)
    {
        release_spinlock(&file->lock);
        return VFS_EINVAL;
    }

    file->offset = (uint64_t) new_offset;
    release_spinlock(&file->lock);

    return new_offset;
}
