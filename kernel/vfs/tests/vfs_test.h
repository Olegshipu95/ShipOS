//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef VFS_TEST_H
#define VFS_TEST_H

// Run all VFS tests
void run_vfs_tests(void);

// Individual tests
void test_vfs_basic(void);
void test_vfs_directories(void);
void test_vfs_seek(void);
void test_vfs_multiple_files(void);

#endif // VFS_TEST_H
