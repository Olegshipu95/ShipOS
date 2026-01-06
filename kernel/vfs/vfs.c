//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file implements the global VFS functions.
//

#include "vfs.h"
#include "../lib/include/panic.h"
#include "../lib/include/string.h"
#include "../lib/include/types.h"
#include "../sync/spinlock.h"
#include "../tty/tty.h"

// Forward declaration
extern void inode_init(void);
extern void mount_init(void);

int vfs_init(void) {
  printf("Initializing VFS...\n");

  // Initialize inode subsystem
  inode_init();

  // Initialize filesystem registration subsystem
  mount_init();

  // Initialize dentry cache
  if (dentry_cache_init() != 0)
  {
    printf("Failed to initialize dentry cache\n");
    return VFS_ERR;
  }

  printf("VFS initialized\n");
  return VFS_OK;
}
