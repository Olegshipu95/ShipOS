//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements filesystem registration and mounting operations.
//

#include "vfs.h"
#include "../lib/include/string.h"
#include "../lib/include/types.h"
#include "../lib/include/hashmap.h"
#include "../sync/spinlock.h"
#include "../tty/tty.h"

#define FILESYSTEM_REGISTRY_BUCKETS 16

// Filesystem registration hashmap
static struct hashmap filesystem_map;
static struct spinlock filesystem_map_lock;
static int filesystem_map_initialized = 0;

// Global root dentry and superblock
static struct dentry *root_dentry = NULL;
static struct superblock *root_sb = NULL;

// Forward declarations'
extern int split_path(const char *path, char components[][MAX_NAME_LEN], int max_components);

void mount_init(void) {
  if (filesystem_map_initialized) {
    return;
  }

  init_spinlock(&filesystem_map_lock, "filesystem_map");
  
  if (hashmap_init(&filesystem_map, FILESYSTEM_REGISTRY_BUCKETS,
                   hashmap_hash_string, hashmap_cmp_string, NULL) != 0) {
    printf("Failed to initialize filesystem registry hashmap\n");
    return;
  }

  filesystem_map_initialized = 1;
}

int vfs_register_filesystem(struct file_system_type *fs_type) {
  if (!fs_type || !fs_type->name || !fs_type->mount) {
    return VFS_EINVAL;
  }

  if (!filesystem_map_initialized) {
    return VFS_ERR;
  }

  acquire_spinlock(&filesystem_map_lock);

  // Check if filesystem with this name is already registered
  struct file_system_type *existing = 
      (struct file_system_type *)hashmap_get(&filesystem_map, fs_type->name);
  if (existing) {
    release_spinlock(&filesystem_map_lock);
    return VFS_EEXIST;
  }

  // Add to hashmap (key is the name string, value is the filesystem type pointer)
  if (hashmap_insert(&filesystem_map, (void *)fs_type->name, fs_type) != 0) {
    release_spinlock(&filesystem_map_lock);
    return VFS_ENOMEM;
  }

  release_spinlock(&filesystem_map_lock);

  printf("Registered filesystem: %s\n", fs_type->name);
  return VFS_OK;
}

int vfs_unregister_filesystem(const char *name) {
  if (!name) {
    return VFS_EINVAL;
  }

  if (!filesystem_map_initialized) {
    return VFS_ERR;
  }

  acquire_spinlock(&filesystem_map_lock);

  if (hashmap_remove(&filesystem_map, name) != 0) {
    release_spinlock(&filesystem_map_lock);
    return VFS_ENOENT;
  }

  release_spinlock(&filesystem_map_lock);
  printf("Unregistered filesystem: %s\n", name);
  return VFS_OK;
}

struct file_system_type *vfs_find_filesystem(const char *name) {
  if (!name) {
    return NULL;
  }

  if (!filesystem_map_initialized) {
    return NULL;
  }

  acquire_spinlock(&filesystem_map_lock);
  struct file_system_type *fs = 
      (struct file_system_type *)hashmap_get(&filesystem_map, name);
  release_spinlock(&filesystem_map_lock);

  return fs;
}

struct superblock *vfs_get_superblock(const char *fs_type, const char *dev_name) {
  if (!fs_type) {
    return NULL;
  }

  struct file_system_type *fs = vfs_find_filesystem(fs_type);
  if (!fs) {
    printf("Filesystem type '%s' not found\n", fs_type);
    return NULL;
  }

  return fs->mount(dev_name);
}

int vfs_mount(struct superblock *sb, struct dentry *mount_point) {
  if (!sb || !mount_point) {
    return VFS_EINVAL;
  }

  // Check if mount point is a directory
  if (!mount_point->inode || mount_point->inode->type != INODE_TYPE_DIR) {
    return VFS_ENOTDIR;
  }

  // Check if mount point is already mounted
  acquire_spinlock(&mount_point->lock);
  if (mount_point->mounted_sb) {
    release_spinlock(&mount_point->lock);
    return VFS_EEXIST;
  }

  mount_point->mounted_sb = sb;
  release_spinlock(&mount_point->lock);

  // Set mount point in superblock
  acquire_spinlock(&sb->lock);
  sb->s_mountpoint = mount_point;
  release_spinlock(&sb->lock);

  printf("Mounted filesystem at %s\n", mount_point->name);
  return VFS_OK;
}

int vfs_unmount(struct dentry *mount_point) {
  if (!mount_point) {
    return VFS_EINVAL;
  }

  acquire_spinlock(&mount_point->lock);

  if (!mount_point->mounted_sb) {
    release_spinlock(&mount_point->lock);
    return VFS_EINVAL; // Not mounted
  }

  struct superblock *sb = mount_point->mounted_sb;
  mount_point->mounted_sb = NULL;
  release_spinlock(&mount_point->lock);

  // Clear mount point in superblock
  acquire_spinlock(&sb->lock);
  sb->s_mountpoint = NULL;
  release_spinlock(&sb->lock);

  printf("Unmounted filesystem from %s\n", mount_point->name);
  return VFS_OK;
}

/**
 * Helper function to create a directory if it doesn't exist
 */
static int ensure_directory_exists(struct dentry *parent, const char *name) {
  if (!parent || !name) {
    return VFS_EINVAL;
  }

  // Try to lookup the directory first
  struct dentry *existing = vfs_lookup(parent, name);
  if (existing) {
    // Directory exists, check if it's actually a directory
    if (existing->inode && existing->inode->type == INODE_TYPE_DIR) {
      vfs_put_dentry(existing);
      return VFS_OK;
    }
    // Exists but not a directory
    vfs_put_dentry(existing);
    return VFS_EEXIST;
  }

  // Directory doesn't exist, create it
  struct inode *parent_inode = parent->inode;
  if (!parent_inode || parent_inode->type != INODE_TYPE_DIR) {
    return VFS_ENOTDIR;
  }

  if (!parent_inode->i_op || !parent_inode->i_op->mkdir) {
    return VFS_EINVAL;
  }

  int ret = parent_inode->i_op->mkdir(parent_inode, name);
  return ret;
}

struct dentry *vfs_get_root(void) {
  return root_dentry;
}

/**
 * @brief Mounts root filesystem.
 * @note This function should be called after vfs_init() and after registering filesystems
 */
int vfs_mount_root(const char *fs_type, const char *dev_name) {
  if (!fs_type) {
    return VFS_EINVAL;
  }

  // Mount root filesystem
  root_sb = vfs_get_superblock(fs_type, dev_name);
  if (!root_sb) {
    printf("Failed to mount root filesystem '%s'\n", fs_type);
    return VFS_ERR;
  }

  // Create root dentry
  root_dentry = vfs_alloc_dentry("/", root_sb->s_root);
  if (!root_dentry) {
    printf("Failed to create root dentry\n");
    return VFS_ERR;
  }

  printf("Root filesystem '%s' mounted at /\n", fs_type);
  return VFS_OK;
}

/**
 * Mount a filesystem at a specified path.
 * Creates all necessary nested directories if they don't exist.
 */
int vfs_mount_at(const char *mount_path, const char *fs_name, const char *dev_name) {
  if (!mount_path || !fs_name) {
    return VFS_EINVAL;
  }

  // Get root dentry
  struct dentry *root = vfs_get_root();
  if (!root) {
    return VFS_ERR;
  }

  // Handle root mount point
  if (strcmp(mount_path, "/") == 0) {
    // ! NOTE: Use vfs_mount_root instead
    return VFS_EINVAL;
  }

  // Split path into components
  char components[32][MAX_NAME_LEN];
  int num_components = split_path(mount_path, components, 32);

  if (num_components == 0) {
    return VFS_EINVAL;
  }

  // Walk through path, creating directories as needed
  struct dentry *current = root;
  vfs_get_dentry(current);

  // Process all components except the last one (the mount point itself)
  for (int i = 0; i < num_components - 1; i++) {
    // Check if current dentry is a mount point
    acquire_spinlock(&current->lock);
    struct superblock *mounted_sb = current->mounted_sb;
    release_spinlock(&current->lock);

    if (mounted_sb && mounted_sb->s_root) {
      // Switch to the root of the mounted filesystem
      struct dentry *mounted_root = vfs_alloc_dentry(current->name, mounted_sb->s_root);
      if (!mounted_root) {
        vfs_put_dentry(current);
        return VFS_ENOMEM;
      }
      mounted_root->parent = current;
      vfs_put_dentry(current);
      current = mounted_root;
    }

    // Try to lookup next component
    struct dentry *next = vfs_lookup(current, components[i]);
    if (next) {
      // Component exists, check if it's a directory
      if (!next->inode || next->inode->type != INODE_TYPE_DIR) {
        vfs_put_dentry(next);
        vfs_put_dentry(current);
        return VFS_ENOTDIR;
      }
      vfs_put_dentry(current);
      current = next;
    } else {
      // Component doesn't exist, create it
      int ret = ensure_directory_exists(current, components[i]);
      if (ret != VFS_OK) {
        vfs_put_dentry(current);
        return ret;
      }

      // Lookup the created directory
      next = vfs_lookup(current, components[i]);
      if (!next) {
        vfs_put_dentry(current);
        return VFS_ERR;
      }
      vfs_put_dentry(current);
      current = next;
    }
  }

  // Handle the mount point itself
  acquire_spinlock(&current->lock);
  struct superblock *mounted_sb = current->mounted_sb;
  release_spinlock(&current->lock);

  if (mounted_sb && mounted_sb->s_root) {
    // Switch to the root of the mounted filesystem
    struct dentry *mounted_root = vfs_alloc_dentry(current->name, mounted_sb->s_root);
    if (!mounted_root) {
      vfs_put_dentry(current);
      return VFS_ENOMEM;
    }
    mounted_root->parent = current;
    vfs_put_dentry(current);
    current = mounted_root;
  }

  // Lookup the mount point directory
  struct dentry *mount_point = vfs_lookup(current, components[num_components - 1]);
  if (!mount_point) {
    // Mount point doesn't exist, create it
    int ret = ensure_directory_exists(current, components[num_components - 1]);
    if (ret != VFS_OK) {
      vfs_put_dentry(current);
      return ret;
    }

    // Lookup the newly created mount point
    mount_point = vfs_lookup(current, components[num_components - 1]);
    if (!mount_point) {
      vfs_put_dentry(current);
      return VFS_ERR;
    }
  }

  vfs_put_dentry(current);

  // Get superblock for the filesystem
  struct superblock *sb = vfs_get_superblock(fs_name, dev_name);
  if (!sb) {
    vfs_put_dentry(mount_point);
    return VFS_ERR;
  }

  // Mount the filesystem
  int ret = vfs_mount(sb, mount_point);
  vfs_put_dentry(mount_point);

  if (ret == VFS_OK) {
    printf("Mounted filesystem '%s' at %s\n", fs_name, mount_path);
  }

  return ret;
}
