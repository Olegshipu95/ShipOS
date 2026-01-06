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
#include "../../tty/tty.h"

int vfs_mount_from_config(void) {
  // Find root filesystem configuration
  struct vfs_mount_config *root_config = NULL;
  for (size_t i = 0; i < vfs_mount_configs_count; i++) {
    if (strcmp(vfs_mount_configs[i].mount_point, "/") == 0) {
      root_config = &vfs_mount_configs[i];
      break;
    }
  }

  if (!root_config) {
    printf("Root filesystem not found in configuration\n");
    serial_printf("[FILESYSTEM] Error: Root filesystem not found in configuration\r\n");
    return VFS_ERR;
  }

  // Mount root filesystem
  serial_printf("[FILESYSTEM] Mounting root filesystem '%s'...\r\n", root_config->fs_type);
  if (vfs_mount_root(root_config->fs_type, root_config->device) != VFS_OK) {
    printf("Failed to mount root filesystem '%s'\n", root_config->fs_type);
    serial_printf("[FILESYSTEM] Error: Failed to mount root filesystem '%s'\r\n", root_config->fs_type);
    return VFS_ERR;
  }
  serial_printf("[FILESYSTEM] Root filesystem '%s' mounted successfully\r\n", root_config->fs_type);

  // Mount other filesystems from configuration
  for (size_t i = 0; i < vfs_mount_configs_count; i++) {
    struct vfs_mount_config *config = &vfs_mount_configs[i];
    
    // Skip root filesystem (already mounted)
    if (strcmp(config->mount_point, "/") == 0) {
      continue;
    }

    serial_printf("[FILESYSTEM] Mounting '%s' at '%s'", 
                  config->fs_type, config->mount_point);
    if (config->device) {
      serial_printf(" (device: %s)", config->device);
    }
    serial_printf("...\r\n");
    
    if (vfs_mount_at(config->mount_point, config->fs_type, config->device) != VFS_OK) {
      printf("Failed to mount filesystem '%s' at '%s'\n", 
             config->fs_type, config->mount_point);
      serial_printf("[FILESYSTEM] Warning: Failed to mount '%s' at '%s'\r\n",
                    config->fs_type, config->mount_point);
    } else {
      serial_printf("[FILESYSTEM] Successfully mounted '%s' at '%s'\r\n",
                    config->fs_type, config->mount_point);
    }
  }

  return VFS_OK;
}

