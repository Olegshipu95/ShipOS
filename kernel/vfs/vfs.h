//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// VFS (Virtual File System) is a layer in the kernel that provides a unified interface for working with different file systems.
// References:
// - https://www.kernel.org/doc/html/latest/filesystems/index.html
// - https://en.wikipedia.org/wiki/Virtual_File_System
//

#ifndef VFS_H
#define VFS_H

#include "../list/list.h"
#include "../sync/spinlock.h"
#include "../lib/include/hashmap.h"
#include <inttypes.h>
#include <stdbool.h>

// TODO: dynamically allocate buffers for names and paths
#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 4096

// Types of inodes
enum inode_type
{
    INODE_TYPE_FILE,
    INODE_TYPE_DIR,
    INODE_TYPE_DEV,
    INODE_TYPE_SYMLINK
};

// File open flags
#define O_RDONLY 0x0001
#define O_WRONLY 0x0002
#define O_RDWR 0x0003
#define O_CREAT 0x0100
#define O_TRUNC 0x0200
#define O_APPEND 0x0400

// Seek modes
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Error codes
#define VFS_OK 0
#define VFS_ERR -1
#define VFS_ENOENT -2    // No such file or directory
#define VFS_EEXIST -3    // File exists
#define VFS_ENOTDIR -4   // Not a directory
#define VFS_EISDIR -5    // Is a directory
#define VFS_EINVAL -6    // Invalid argument
#define VFS_ENOMEM -7    // Out of memory
#define VFS_ENOTEMPTY -8 // Directory is not empty

// Forward declarations
struct inode;
struct dentry;
struct file;
struct superblock;

// Inode operations
struct inode_operations
{
    struct inode *(*lookup)(struct inode *dir, const char *name);
    int (*create)(struct inode *dir, const char *name, struct inode **result);
    int (*mkdir)(struct inode *dir, const char *name);
    int (*unlink)(struct inode *dir, const char *name);
};

// File operations
struct file_operations
{
    int64_t (*read)(struct file *file, char *buf, uint64_t count);
    int64_t (*write)(struct file *file, const char *buf, uint64_t count);
    int (*open)(struct inode *inode, struct file *file);
    int (*close)(struct file *file);
    int64_t (*lseek)(struct file *file, int64_t offset, int whence);
    int (*readdir)(struct file *file, struct dirent *dirent, uint64_t count);
};

// Superblock operations
struct superblock_operations
{
    struct inode *(*alloc_inode)(struct superblock *sb);
    void (*destroy_inode)(struct inode *inode);
    int (*sync_fs)(struct superblock *sb);
};

// File system type structure
struct file_system_type
{
    const char *name;                                  // Name of the filesystem (e.g., "tmpfs", "ext2")
    struct superblock *(*mount)(const char *dev_name); // Mount function
    struct list list_node;                             // For filesystem registration list
};

// Inode structure
struct inode
{
    uint64_t ino;                  // Inode number
    enum inode_type type;          // Type (file, dir, etc)
    uint32_t nlink;                // Hard link count
    uint64_t size;                 // File size in bytes
    void *fs_private;              // FS-specific private data
    struct inode_operations *i_op; // Inode operations
    struct file_operations *f_op;  // File operations
    struct superblock *sb;         // Superblock
    struct spinlock lock;          // Lock for this inode
    uint32_t ref;                  // Reference count
};

// Directory entry
struct dentry
{
    char name[MAX_NAME_LEN];       // Name
    struct inode *inode;           // Associated inode
    struct dentry *parent;         // Parent dentry
    struct list children;          // List of child dentries
    struct list sibling;           // For parent's children list
    struct spinlock lock;          // Lock
    uint32_t ref;                  // Reference count
    struct superblock *mounted_sb; // Mounted filesystem (if this is a mount point)
};

// Open file structure
struct file
{
    struct inode *inode;          // Associated inode
    struct dentry *dentry;        // Associated dentry
    uint64_t offset;              // Current read/write position
    int flags;                    // Open flags
    struct file_operations *f_op; // File operations
    uint32_t ref;                 // Reference count
    struct spinlock lock;         // Lock
};

// Directory entry structure
struct dirent
{
    char d_name[MAX_NAME_LEN];
    uint64_t d_ino;
    enum inode_type d_type;
};

// Superblock
struct superblock
{
    uint64_t s_magic;                   // Magic number
    struct inode *s_root;               // Root inode
    struct superblock_operations *s_op; // Operations
    void *s_fs_info;                    // FS-specific info
    struct spinlock lock;               // Lock
    struct dentry *s_mountpoint;        // Dentry where this filesystem is mounted
};

// VFS initialization
int vfs_init(void);
int vfs_mount_root(const char *fs_type, const char *dev_name);

// Inode management
struct inode *vfs_alloc_inode(struct superblock *sb);
void vfs_free_inode(struct inode *inode);
struct inode *vfs_get_inode(struct inode *inode);
void vfs_put_inode(struct inode *inode);

// Dentry management
struct dentry *vfs_alloc_dentry(const char *name, struct inode *inode);
void vfs_free_dentry(struct dentry *dentry);
struct dentry *vfs_get_dentry(struct dentry *dentry);
void vfs_put_dentry(struct dentry *dentry);
struct dentry *vfs_lookup(struct dentry *parent, const char *name);

// File management
struct file *vfs_alloc_file(void);
void vfs_free_file(struct file *file);
int vfs_open(const char *path, int flags, struct file **result);
int vfs_close(struct file *file);
int64_t vfs_read(struct file *file, char *buf, uint64_t count);
int64_t vfs_write(struct file *file, const char *buf, uint64_t count);
int64_t vfs_lseek(struct file *file, int64_t offset, int whence);
int vfs_unlink(const char *path);

// Directory management
int vfs_opendir(const char *path, struct file **result);
int vfs_readdir(struct file *file, struct dirent *dirent, uint64_t count);

// Dentry cache management
extern int dentry_cache_initialized;
int dentry_cache_init(void);
void dentry_cache_destroy(void);
struct dentry *dentry_cache_lookup(struct inode *parent, const char *name);
void dentry_cache_add(struct dentry *dentry);
void dentry_cache_remove(struct dentry *dentry);

// Path resolution
struct dentry *vfs_path_lookup(const char *path);

// Get root dentry
struct dentry *vfs_get_root(void);

// Filesystem registration
int vfs_register_filesystem(struct file_system_type *fs_type);
int vfs_unregister_filesystem(const char *name);
struct file_system_type *vfs_find_filesystem(const char *name);

// Mount/unmount operations
int vfs_mount(struct superblock *sb, struct dentry *mount_point);
int vfs_unmount(struct dentry *mount_point);
struct superblock *vfs_get_superblock(const char *fs_type, const char *dev_name);
int vfs_mount_at(const char *mount_path, const char *fs_name, const char *dev_name);

// Mount filesystems from configuration file
int vfs_mount_from_config(void);

#endif // VFS_H
