//
// Created by ShipOS developers.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements filesystem mounting from configuration file.
//

#include "vfs_config.h"
#include "../vfs.h"
#include "../../lib/include/panic.h"
#include "../../lib/include/string.h"
#include "../../lib/include/logging.h"
#include "../../tty/tty.h"

int vfs_mount_from_config(void)
{
    // Find root filesystem configuration
    struct vfs_mount_config *root_config = NULL;
    for (size_t i = 0; i < vfs_mount_configs_count; i++)
    {
        if (strcmp(vfs_mount_configs[i].mount_point, "/") == 0)
        {
            root_config = &vfs_mount_configs[i];
            break;
        }
    }

    if (!root_config)
    {
        printf("Root filesystem not found in configuration\n");
        LOG_SERIAL("FILESYSTEM", "Error: Root filesystem not found in configuration");
        return VFS_ERR;
    }

    // Mount root filesystem
    LOG_SERIAL("FILESYSTEM", "Mounting root filesystem '%s'...", root_config->fs_type);
    if (vfs_mount_root(root_config->fs_type, root_config->device) != VFS_OK)
    {
        printf("Failed to mount root filesystem '%s'\n", root_config->fs_type);
        LOG_SERIAL("FILESYSTEM", "Error: Failed to mount root filesystem '%s'", root_config->fs_type);
        return VFS_ERR;
    }
    LOG_SERIAL("FILESYSTEM", "Root filesystem '%s' mounted successfully", root_config->fs_type);

    // Mount other filesystems from configuration
    for (size_t i = 0; i < vfs_mount_configs_count; i++)
    {
        struct vfs_mount_config *config = &vfs_mount_configs[i];

        // Skip root filesystem (already mounted)
        if (strcmp(config->mount_point, "/") == 0)
        {
            continue;
        }

        if (config->device)
        {
            LOG_SERIAL("FILESYSTEM", "Mounting '%s' at '%s' (device: %s)...", config->fs_type, config->mount_point, config->device);
        }
        else
        {
            LOG_SERIAL("FILESYSTEM", "Mounting '%s' at '%s'...", config->fs_type, config->mount_point);
        }

        if (vfs_mount_at(config->mount_point, config->fs_type, config->device) != VFS_OK)
        {
            printf("Failed to mount filesystem '%s' at '%s'\n",
                   config->fs_type, config->mount_point);
            LOG_SERIAL("FILESYSTEM", "Warning: Failed to mount '%s' at '%s'",
                       config->fs_type, config->mount_point);
        }
        else
        {
            LOG_SERIAL("FILESYSTEM", "Successfully mounted '%s' at '%s'",
                       config->fs_type, config->mount_point);
        }
    }

    return VFS_OK;
}
