//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "tmpfs.h"
#include "../../kalloc/kalloc.h"
#include "../../lib/include/memset.h"
#include "../../lib/include/string.h"
#include "../../memlayout.h"
#include "../../tty/tty.h"

/**
 * Gets the container structure from a pointer to its member.
 * @param ptr Pointer to the member.
 * @param type Type of the container structure.
 * @param member Name of the member.
 * @return Pointer to the container structure.
 **/
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - __builtin_offsetof(type, member)))

// Forward declarations
static int64_t tmpfs_read(struct file *file, char *buf, uint64_t count);
static int64_t tmpfs_write(struct file *file, const char *buf, uint64_t count);
static int tmpfs_open(struct inode *inode, struct file *file);
static int tmpfs_close(struct file *file);

static struct inode *tmpfs_lookup(struct inode *dir, const char *name);
static int tmpfs_create(struct inode *dir, const char *name, struct inode **result);
static int tmpfs_mkdir(struct inode *dir, const char *name);
static int tmpfs_unlink(struct inode *dir, const char *name);

static struct inode *tmpfs_alloc_inode(struct superblock *sb);
static void tmpfs_destroy_inode(struct inode *inode);

// File operations for regular files
struct file_operations tmpfs_file_ops = {
    .read = tmpfs_read,
    .write = tmpfs_write,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .lseek = NULL // Use default VFS implementation
};

/**
 * @brief Inode operations for regular files
 *
 * @note At the moment, this is a placeholder. Later can be used for functions like:
 * - getattr
 * - chmod
 * - truncate
 */
struct inode_operations tmpfs_file_inode_ops = {
    .lookup = NULL,
    .create = NULL,
    .mkdir = NULL,
    .unlink = NULL,
    .rmdir = NULL};

// Inode operations for directories
struct inode_operations tmpfs_dir_inode_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
    .unlink = tmpfs_unlink,
    .rmdir = NULL};

/**
 * @brief File operations for directories
 *
 * @note At the moment, this is a placeholder. Later can be used for functions like:
 * - readdir
 */
struct file_operations tmpfs_dir_file_ops = {
    .read = NULL,
    .write = NULL,
    .open = NULL,
    .close = NULL,
    .lseek = NULL};

// Superblock operations
struct superblock_operations tmpfs_sb_ops = {
    .alloc_inode = tmpfs_alloc_inode,
    .destroy_inode = tmpfs_destroy_inode,
    .sync_fs = NULL};

// Read from tmpfs file
static int64_t tmpfs_read(struct file *file, char *buf, uint64_t count)
{
    struct inode *inode = file->inode;
    struct tmpfs_inode_info *info = inode->fs_private;

    if (!info || !info->data)
    {
        return 0;
    }

    acquire_spinlock(&inode->lock);

    // Check bounds
    if (file->offset >= inode->size)
    {
        release_spinlock(&inode->lock);
        return 0;
    }

    // Adjust count
    if (file->offset + count > inode->size)
    {
        count = inode->size - file->offset;
    }

    // Copy data
    memcpy(buf, (char *) info->data + file->offset, count);
    file->offset += count;

    release_spinlock(&inode->lock);
    return count;
}

// Write to tmpfs file
static int64_t tmpfs_write(struct file *file, const char *buf, uint64_t count)
{
    struct inode *inode = file->inode;
    struct tmpfs_inode_info *info = inode->fs_private;

    if (!info)
    {
        return VFS_EINVAL;
    }

    acquire_spinlock(&inode->lock);

    uint64_t new_size = file->offset + count;

    // If we need more memory - allocate
    if (new_size > info->data_size)
    {
        uint64_t pages_needed = PGROUNDUP(new_size) / PGSIZE;

        // Allocate new memory
        void *new_data = NULL;
        for (uint64_t i = 0; i < pages_needed; i++)
        {
            void *page = kalloc();
            if (!page)
            {
                release_spinlock(&inode->lock);
                return VFS_ENOMEM;
            }
            if (i == 0)
            {
                new_data = page;
            }
        }

        // Copy old data if exists
        if (info->data && inode->size > 0)
        {
            memcpy(new_data, info->data, inode->size);

            // Free old pages
            uint64_t old_pages = info->data_size / PGSIZE;
            for (uint64_t i = 0; i < old_pages; i++)
            {
                kfree((char *) info->data + i * PGSIZE);
            }
        }

        info->data = new_data;
        info->data_size = pages_needed * PGSIZE;
    }

    // Write data
    memcpy((char *) info->data + file->offset, buf, count);
    file->offset += count;

    if (file->offset > inode->size)
    {
        inode->size = file->offset;
    }

    release_spinlock(&inode->lock);
    return count;
}

static int tmpfs_open(struct inode *inode, struct file *file)
{
    // Nothing special
    return VFS_OK;
}

static int tmpfs_close(struct file *file)
{
    // Nothing special
    return VFS_OK;
}

// Lookup file in directory
static struct inode *tmpfs_lookup(struct inode *dir, const char *name)
{
    // Check arguments
    if (!dir || dir->type != INODE_TYPE_DIR)
    {
        return NULL;
    }

    struct tmpfs_inode_info *dir_info = dir->fs_private;
    if (!dir_info)
    {
        return NULL;
    }

    acquire_spinlock(&dir->lock);

    // Search through directory entries
    struct list *node;
    for (node = dir_info->entries.next; node != &dir_info->entries; node = node->next)
    {
        struct tmpfs_dir_entry *entry = container_of(node, struct tmpfs_dir_entry, list_node);

        if (strcmp(entry->name, name) == 0)
        {
            struct inode *found = entry->inode;
            vfs_get_inode(found);
            release_spinlock(&dir->lock);
            return found;
        }
    }

    release_spinlock(&dir->lock);
    return NULL;
}

/**
 * @brief Create new file in directory
 *
 * @note Uses same logic with `struct inode **result` as `vfs_open`
 */
static int tmpfs_create(struct inode *dir, const char *name, struct inode **result)
{
    if (!dir || dir->type != INODE_TYPE_DIR || !name || !result)
    {
        return VFS_EINVAL;
    }

    struct tmpfs_inode_info *dir_info = dir->fs_private;
    if (!dir_info)
    {
        return VFS_EINVAL;
    }

    // Check if file already exists
    struct inode *existing = tmpfs_lookup(dir, name);
    if (existing)
    {
        vfs_put_inode(existing);
        return VFS_EEXIST;
    }

    // Create new inode
    struct inode *new_inode = tmpfs_alloc_inode(dir->sb);
    if (!new_inode)
    {
        return VFS_ENOMEM;
    }

    new_inode->type = INODE_TYPE_FILE;
    new_inode->size = 0;
    new_inode->i_op = &tmpfs_file_inode_ops;
    new_inode->f_op = &tmpfs_file_ops;

    // Create directory entry
    struct tmpfs_dir_entry *entry = kalloc();
    if (!entry)
    {
        tmpfs_destroy_inode(new_inode);
        return VFS_ENOMEM;
    }

    strncpy(entry->name, name, MAX_NAME_LEN - 1);
    entry->name[MAX_NAME_LEN - 1] = '\0';
    entry->inode = new_inode;
    vfs_get_inode(new_inode);

    // Add to directory
    acquire_spinlock(&dir->lock);
    lst_push(&dir_info->entries, &entry->list_node);
    release_spinlock(&dir->lock);

    *result = new_inode;
    return VFS_OK;
}

// Create new directory
static int tmpfs_mkdir(struct inode *dir, const char *name)
{
    if (!dir || dir->type != INODE_TYPE_DIR || !name)
    {
        return VFS_EINVAL;
    }

    struct tmpfs_inode_info *dir_info = dir->fs_private;
    if (!dir_info)
    {
        return VFS_EINVAL;
    }

    // Check if directory already exists
    struct inode *existing = tmpfs_lookup(dir, name);
    if (existing)
    {
        vfs_put_inode(existing);
        return VFS_EEXIST;
    }

    // Create new inode
    struct inode *new_inode = tmpfs_alloc_inode(dir->sb);
    if (!new_inode)
    {
        return VFS_ENOMEM;
    }

    new_inode->type = INODE_TYPE_DIR;
    new_inode->size = 0;
    new_inode->i_op = &tmpfs_dir_inode_ops;
    new_inode->f_op = &tmpfs_dir_file_ops;

    // Create directory entry
    struct tmpfs_dir_entry *entry = kalloc();
    if (!entry)
    {
        tmpfs_destroy_inode(new_inode);
        return VFS_ENOMEM;
    }

    strncpy(entry->name, name, MAX_NAME_LEN - 1);
    entry->name[MAX_NAME_LEN - 1] = '\0';
    entry->inode = new_inode;
    vfs_get_inode(new_inode);

    // Add to parent directory
    acquire_spinlock(&dir->lock);
    lst_push(&dir_info->entries, &entry->list_node);
    release_spinlock(&dir->lock);

    return VFS_OK;
}

// Unlink (delete) file from directory
static int tmpfs_unlink(struct inode *dir, const char *name)
{
    if (!dir || dir->type != INODE_TYPE_DIR || !name)
    {
        return VFS_EINVAL;
    }

    struct tmpfs_inode_info *dir_info = dir->fs_private;
    if (!dir_info)
    {
        return VFS_EINVAL;
    }

    acquire_spinlock(&dir->lock);

    // Find entry
    struct list *node;
    for (node = dir_info->entries.next; node != &dir_info->entries; node = node->next)
    {
        struct tmpfs_dir_entry *entry = container_of(node, struct tmpfs_dir_entry, list_node);

        if (strcmp(entry->name, name) == 0)
        {
            // Remove from list
            lst_remove(&entry->list_node);

            // Release inode reference
            vfs_put_inode(entry->inode);

            // Free entry
            kfree(entry);

            release_spinlock(&dir->lock);
            return VFS_OK;
        }
    }

    release_spinlock(&dir->lock);
    return VFS_ENOENT;
}

// Allocate new inode
static struct inode *tmpfs_alloc_inode(struct superblock *sb)
{
    struct inode *inode = vfs_alloc_inode(sb);
    if (!inode)
    {
        return NULL;
    }

    // Allocate tmpfs-specific data
    struct tmpfs_inode_info *info = kalloc();
    if (!info)
    {
        vfs_free_inode(inode);
        return NULL;
    }

    memset(info, 0, sizeof(struct tmpfs_inode_info));
    lst_init(&info->entries);

    inode->fs_private = info;
    return inode;
}

// Destroy inode
static void tmpfs_destroy_inode(struct inode *inode)
{
    if (!inode)
        return;

    struct tmpfs_inode_info *info = inode->fs_private;
    if (info)
    {
        // Free data if it's a file
        if (inode->type == INODE_TYPE_FILE && info->data)
        {
            uint64_t pages = info->data_size / PGSIZE;
            for (uint64_t i = 0; i < pages; i++)
            {
                kfree((char *) info->data + i * PGSIZE);
            }
        }

        // Free directory entries if it's a directory
        if (inode->type == INODE_TYPE_DIR)
        {
            struct list *node = info->entries.next;
            while (node != &info->entries)
            {
                struct list *next = node->next;
                struct tmpfs_dir_entry *entry = container_of(node, struct tmpfs_dir_entry, list_node);

                vfs_put_inode(entry->inode);
                kfree(entry);
                node = next;
            }
        }

        kfree(info);
    }
}

// Mount tmpfs
struct superblock *tmpfs_mount(void)
{
    struct superblock *sb = kalloc();
    if (!sb)
    {
        return NULL;
    }

    memset(sb, 0, sizeof(struct superblock));

    sb->s_magic = TMPFS_MAGIC;
    sb->s_op = &tmpfs_sb_ops;
    init_spinlock(&sb->lock, "tmpfs_sb");

    // Allocate filesystem info
    struct tmpfs_fs_info *fs_info = kalloc();
    if (!fs_info)
    {
        kfree(sb);
        return NULL;
    }

    memset(fs_info, 0, sizeof(struct tmpfs_fs_info));
    sb->s_fs_info = fs_info;

    // Create root inode
    struct inode *root = tmpfs_alloc_inode(sb);
    if (!root)
    {
        kfree(fs_info);
        kfree(sb);
        return NULL;
    }

    root->type = INODE_TYPE_DIR;
    root->i_op = &tmpfs_dir_inode_ops;
    root->f_op = &tmpfs_dir_file_ops;

    sb->s_root = root;

    printf("tmpfs mounted successfully\n");
    return sb;
}

int tmpfs_unmount(struct superblock *sb)
{
    if (!sb)
    {
        return VFS_EINVAL;
    }

    // Free root inode
    if (sb->s_root)
    {
        tmpfs_destroy_inode(sb->s_root);
        vfs_free_inode(sb->s_root);
    }

    // Free fs info
    if (sb->s_fs_info)
    {
        kfree(sb->s_fs_info);
    }

    kfree(sb);
    return VFS_OK;
}

int tmpfs_init(void)
{
    // Nothing special
    printf("tmpfs initialized\n");
    return VFS_OK;
}
