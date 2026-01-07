//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "tmpfs.h"
#include "../../kalloc/kalloc.h"
#include "../../lib/include/memset.h"
#include "../../lib/include/string.h"
#include "../../lib/include/types.h"
#include "../../lib/include/hashmap.h"
#include "../../memlayout.h"
#include "../../tty/tty.h"
#include "../../vfs/vfs.h"

// Forward declarations
static int64_t tmpfs_read(struct file *file, char *buf, uint64_t count);
static int64_t tmpfs_write(struct file *file, const char *buf, uint64_t count);
static int tmpfs_open(struct inode *inode, struct file *file);
static int tmpfs_close(struct file *file);
static int tmpfs_readdir(struct file *file, struct dirent *dirent, uint64_t count);

static struct inode *tmpfs_lookup(struct inode *dir, const char *name);
static int tmpfs_create(struct inode *dir, const char *name, struct inode **result);
static int tmpfs_mkdir(struct inode *dir, const char *name);
static int tmpfs_unlink(struct inode *dir, const char *name);

static struct inode *tmpfs_alloc_inode(struct superblock *sb);
static void tmpfs_destroy_inode(struct inode *inode);
static int tmpfs_create_child_inode(struct inode *dir, const char *name, struct inode **result);

static int tmpfs_create_entry(struct inode *dir, const char *name, struct inode *child);
static int tmpfs_remove_entry(struct inode *dir, const char *name);

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
    .unlink = NULL};

// Inode operations for directories
struct inode_operations tmpfs_dir_inode_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
    .unlink = tmpfs_unlink};

/**
 * @brief File operations for directories
 */
struct file_operations tmpfs_dir_file_ops = {
    .read = NULL,
    .write = NULL,
    .open = NULL,
    .close = NULL,
    .lseek = NULL,
    .readdir = tmpfs_readdir};

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

// Read directory entries
static int tmpfs_readdir(struct file *file, struct dirent *dirent, uint64_t count)
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

    struct tmpfs_inode_info *dir_info = inode->fs_private;
    if (!dir_info)
    {
        return VFS_EINVAL;
    }

    acquire_spinlock(&inode->lock);

    // Find starting position based on file->offset
    struct list *node = dir_info->entries_list.next;
    uint64_t current_pos = 0;

    // Skip entries until we reach the offset
    while (node != &dir_info->entries_list && current_pos < file->offset)
    {
        node = node->next;
        current_pos++;
    }

    // Read entries
    uint64_t entries_read = 0;
    while (node != &dir_info->entries_list && entries_read < count)
    {
        struct tmpfs_dir_entry *entry = container_of(node, struct tmpfs_dir_entry, list_node);

        // Fill dirent structure
        strncpy(dirent[entries_read].d_name, entry->name, MAX_NAME_LEN - 1);
        dirent[entries_read].d_name[MAX_NAME_LEN - 1] = '\0';
        dirent[entries_read].d_ino = entry->inode->ino;
        dirent[entries_read].d_type = entry->inode->type;

        entries_read++;
        node = node->next;
    }

    // Update file offset
    file->offset += entries_read;

    release_spinlock(&inode->lock);

    // Return number of entries read (positive value)
    // Negative values are error codes
    return (int) entries_read;
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

    // Lookup in hashmap
    struct tmpfs_dir_entry *entry = (struct tmpfs_dir_entry *) hashmap_get(&dir_info->entries, (void *) name);
    if (entry)
    {
        struct inode *found = entry->inode;
        vfs_get_inode(found);
        release_spinlock(&dir->lock);
        return found;
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
    struct inode *child_inode;
    int ret = tmpfs_create_child_inode(dir, name, &child_inode);
    if (ret != VFS_OK) {
        return ret;
    }

    child_inode->type = INODE_TYPE_FILE;
    child_inode->size = 0;
    child_inode->i_op = &tmpfs_file_inode_ops;
    child_inode->f_op = &tmpfs_file_ops;

    ret = tmpfs_create_entry(dir, name, child_inode);
    if (ret != VFS_OK) {
        tmpfs_destroy_inode(child_inode);
        vfs_free_inode(child_inode);
        return ret;
    }

    *result = child_inode;
    return VFS_OK;
}

// Create new directory
static int tmpfs_mkdir(struct inode *dir, const char *name)
{
    struct inode *child_inode;
    int ret = tmpfs_create_child_inode(dir, name, &child_inode);
    if (ret != VFS_OK) {
        return ret;
    }

    child_inode->type = INODE_TYPE_DIR;
    child_inode->size = 0;
    child_inode->i_op = &tmpfs_dir_inode_ops;
    child_inode->f_op = &tmpfs_dir_file_ops;

    ret = tmpfs_create_entry(dir, name, child_inode);
    if (ret != VFS_OK) {
        tmpfs_destroy_inode(child_inode);
        vfs_free_inode(child_inode);
        return ret;
    }

    return VFS_OK;
}

// Unlink (delete) file or directory from directory
static int tmpfs_unlink(struct inode *dir, const char *name)
{
    return tmpfs_remove_entry(dir, name);
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
    struct tmpfs_inode_info *info = kzalloc(sizeof(struct tmpfs_inode_info));
    if (!info)
    {
        vfs_free_inode(inode);
        return NULL;
    }

    // Initialize hashmap and list for directories
    lst_init(&info->entries_list);
    if (hashmap_init(&info->entries, TMPFS_DIR_BUCKETS, hashmap_hash_string, hashmap_cmp_string, NULL) != 0)
    {
        kfree(info);
        vfs_free_inode(inode);
        return NULL;
    }

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
            // Clear all entries from list (hashmap will be cleared automatically)
            struct list *node = info->entries_list.next;
            while (node != &info->entries_list)
            {
                struct list *next = node->next;
                struct tmpfs_dir_entry *entry = container_of(node, struct tmpfs_dir_entry, list_node);

                vfs_put_inode(entry->inode);
                kfree(entry);
                node = next;
            }

            // Destroy hashmap
            hashmap_destroy(&info->entries);
        }

        kfree(info);
    }
}

// Create child inode in specified directory
static int tmpfs_create_child_inode(struct inode *dir, const char *name, struct inode **result)
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

    *result = new_inode;
    return VFS_OK;
}

// Create entry in directory
static int tmpfs_create_entry(struct inode *dir, const char *name, struct inode *child)
{
    struct tmpfs_inode_info *dir_info = dir->fs_private;
    if (!dir_info)
    {
        return VFS_EINVAL;
    }

    // Create directory entry
    struct tmpfs_dir_entry *entry = kzalloc(sizeof(struct tmpfs_dir_entry));
    if (!entry)
    {
        tmpfs_destroy_inode(child);
        return VFS_ENOMEM;
    }

    strncpy(entry->name, name, MAX_NAME_LEN - 1);
    entry->name[MAX_NAME_LEN - 1] = '\0';
    entry->inode = child;
    vfs_get_inode(child);

    // Add to parent directory hashmap and list (key is the name string stored in entry)
    acquire_spinlock(&dir->lock);
    if (hashmap_insert(&dir_info->entries, entry->name, entry) != 0)
    {
        release_spinlock(&dir->lock);
        vfs_put_inode(child);
        kfree(entry);
        tmpfs_destroy_inode(child);
        return VFS_ENOMEM;
    }
    lst_push(&dir_info->entries_list, &entry->list_node);
    release_spinlock(&dir->lock);

    return VFS_OK;
}

// Remove entry (file or directory) from directory
static int tmpfs_remove_entry(struct inode *dir, const char *name)
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

    // Find and remove entry from hashmap and list
    struct tmpfs_dir_entry *entry = (struct tmpfs_dir_entry *) hashmap_get(&dir_info->entries, (void *) name);
    if (entry)
    {
        hashmap_remove(&dir_info->entries, entry->name);
        lst_remove(&entry->list_node);

        vfs_put_inode(entry->inode);

        kfree(entry);

        release_spinlock(&dir->lock);
        return VFS_OK;
    }

    release_spinlock(&dir->lock);
    return VFS_ENOENT;
}

// Wrapper for tmpfs_mount that matches file_system_type signature
static struct superblock *tmpfs_mount_impl(const char *dev_name)
{
    // tmpfs doesn't use device name, but we accept it for compatibility
    (void) dev_name;
    return tmpfs_mount();
}

// Mount tmpfs
struct superblock *tmpfs_mount(void)
{
    struct superblock *sb = kzalloc(sizeof(struct superblock));
    if (!sb)
    {
        return NULL;
    }

    sb->s_magic = TMPFS_MAGIC;
    sb->s_op = &tmpfs_sb_ops;
    init_spinlock(&sb->lock, "tmpfs_sb");

    // Allocate filesystem info
    struct tmpfs_fs_info *fs_info = kzalloc(sizeof(struct tmpfs_fs_info));
    if (!fs_info)
    {
        kfree(sb);
        return NULL;
    }
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

// Filesystem type structure for tmpfs
static struct file_system_type tmpfs_fs_type = {
    .name = "tmpfs",
    .mount = tmpfs_mount_impl,
};

int tmpfs_init(void)
{
    // Register tmpfs filesystem
    int ret = vfs_register_filesystem(&tmpfs_fs_type);
    if (ret != VFS_OK)
    {
        printf("Failed to register tmpfs filesystem\n");
        return ret;
    }

    printf("tmpfs initialized and registered\n");
    return VFS_OK;
}
