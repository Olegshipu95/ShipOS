//
// Created by ShipOS developers.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Header for auto-generated configuration VFS mount configuration.
//

#ifndef VFS_CONFIG_H
#define VFS_CONFIG_H

#include <stddef.h>

// Structure for filesystem mount configuration
struct vfs_mount_config
{
    const char *mount_point; // Mount point path (e.g., "/", "/mnt/usb")
    const char *fs_type;     // Filesystem type (e.g., "tmpfs", "ext2")
    const char *device;      // Device name (NULL for filesystems that don't need a device)
};

// Array of mount configurations (terminated with {NULL, NULL, NULL})
extern struct vfs_mount_config vfs_mount_configs[];

// Number of mount configurations (excluding terminator)
extern size_t vfs_mount_configs_count;

#endif // VFS_CONFIG_H
