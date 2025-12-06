//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "../../kalloc/kalloc.h"
#include "../../lib/include/string.h"
#include "../../tty/tty.h"
#include "../vfs.h"

#define TEST_BUFSIZE 100

static int test_vfs_basic(void) {
    struct file *f;
    int ret;
    const char *data = "Hello, tmpfs!";
    char buf[TEST_BUFSIZE];
    int64_t written, read_bytes;

    // Test 1: Create and open a file
    ret = vfs_open("/test.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK) {
        printf("vfs_basic: FAIL vfs_open=%d\n", ret);
        return -1;
    }

    // Test 2: Write to file
    written = vfs_write(f, data, strlen(data));
    if (written != (int64_t)strlen(data)) {
        printf("vfs_basic: FAIL write=%d exp=%d\n", written, strlen(data));
        vfs_close(f);
        return -1;
    }

    // Test 3: Close and reopen file
    vfs_close(f);
    ret = vfs_open("/test.txt", O_RDONLY, &f);
    if (ret != VFS_OK) {
        printf("vfs_basic: FAIL reopen=%d\n", ret);
        return -1;
    }

    // Test 4: Read from file
    memset(buf, 0, TEST_BUFSIZE);
    read_bytes = vfs_read(f, buf, TEST_BUFSIZE);
    printf("[INFO] vfs_basic: read %d bytes: '%s'\n", (int)read_bytes, buf);

    if (read_bytes != (int64_t)strlen(data)) {
        printf("vfs_basic: FAIL read=%d exp=%d\n", read_bytes, strlen(data));
        vfs_close(f);
        return -1;
    }
    if (strcmp(buf, data) != 0) {
        printf("vfs_basic: FAIL data mismatch\n");
        vfs_close(f);
        return -1;
    }

    vfs_close(f);
    return 0;
}

static int test_vfs_directories(void) {
    struct dentry *root;
    struct inode *root_inode;
    struct file *f;
    int ret;
    const char *data = "File in directory!";
    char buf[TEST_BUFSIZE];
    int64_t written, read_bytes;

    // Test 1: Create directory
    root = vfs_get_root();
    if (!root) {
        printf("vfs_directories: FAIL no root\n");
        return -1;
    }

    root_inode = root->inode;
    if (!root_inode || !root_inode->i_op || !root_inode->i_op->mkdir) {
        printf("vfs_directories: FAIL no mkdir op\n");
        return -1;
    }

    ret = root_inode->i_op->mkdir(root_inode, "testdir");
    if (ret != VFS_OK) {
        printf("vfs_directories: FAIL mkdir=%d\n", ret);
        return -1;
    }

    // Test 2: Create file in directory
    ret = vfs_open("/testdir/file.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK) {
        printf("vfs_directories: FAIL create=%d\n", ret);
        return -1;
    }

    // Test 3: Write and read from file in directory
    written = vfs_write(f, data, strlen(data));
    if (written != (int64_t)strlen(data)) {
        printf("vfs_directories: FAIL write\n");
        vfs_close(f);
        return -1;
    }

    vfs_close(f);

    // Reopen and read
    ret = vfs_open("/testdir/file.txt", O_RDONLY, &f);
    if (ret != VFS_OK) {
        printf("vfs_directories: FAIL reopen=%d\n", ret);
        return -1;
    }

    memset(buf, 0, TEST_BUFSIZE);
    read_bytes = vfs_read(f, buf, TEST_BUFSIZE);
    if (read_bytes != (int64_t)strlen(data) || strcmp(buf, data) != 0) {
        printf("vfs_directories: FAIL verify\n");
        vfs_close(f);
        return -1;
    }

    vfs_close(f);
    return 0;
}

static int test_vfs_seek(void) {
    struct file *f;
    int ret;
    const char *data = "0123456789ABCDEF";
    char buf[10];
    int64_t new_pos;

    // Create and write to file
    ret = vfs_open("/seektest.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK) {
        printf("vfs_seek: FAIL create=%d\n", ret);
        return -1;
    }
    vfs_write(f, data, strlen(data));

    // Test SEEK_SET
    new_pos = vfs_lseek(f, 5, SEEK_SET);
    if (new_pos != 5) {
        printf("vfs_seek: FAIL SET pos=%d\n", new_pos);
        vfs_close(f);
        return -1;
    }
    memset(buf, 0, 10);
    vfs_read(f, buf, 5);
    if (strcmp(buf, "56789") != 0) {
        printf("vfs_seek: FAIL SET data='%s'\n", buf);
        vfs_close(f);
        return -1;
    }

    // Test SEEK_CUR
    vfs_lseek(f, -3, SEEK_CUR);
    memset(buf, 0, 10);
    vfs_read(f, buf, 3);
    if (strcmp(buf, "789") != 0) {
        printf("vfs_seek: FAIL CUR data='%s'\n", buf);
        vfs_close(f);
        return -1;
    }

    // Test SEEK_END
    vfs_lseek(f, -4, SEEK_END);
    memset(buf, 0, 10);
    vfs_read(f, buf, 4);
    if (strcmp(buf, "CDEF") != 0) {
        printf("vfs_seek: FAIL END data='%s'\n", buf);
        vfs_close(f);
        return -1;
    }

    vfs_close(f);
    return 0;
}

static int test_vfs_multiple_files(void) {
    int i;
    char path[32];
    char data[64];
    char expected[64];
    char buf[64];
    struct file *f;
    int ret;

    // Create multiple files
    for (i = 0; i < 5; i++) {
        // Will be "/file0.txt", "/file1.txt", etc
        strcpy(path, "/file0.txt");
        path[5] = '0' + i;

        ret = vfs_open(path, O_CREAT | O_RDWR, &f);
        if (ret != VFS_OK) {
            printf("vfs_multiple_files: FAIL create %s=%d\n", path, ret);
            return -1;
        }

        // Will be "File 0", "File 1", etc
        strcpy(data, "File 0");
        data[5] = '0' + i;

        vfs_write(f, data, strlen(data));
        vfs_close(f);
    }

    // Verify all files
    for (i = 0; i < 5; i++) {
        strcpy(path, "/file0.txt");
        path[5] = '0' + i;
        strcpy(expected, "File 0");
        expected[5] = '0' + i;

        ret = vfs_open(path, O_RDONLY, &f);
        if (ret != VFS_OK) {
            printf("vfs_multiple_files: FAIL open %s=%d\n", path, ret);
            return -1;
        }

        memset(buf, 0, 64);
        vfs_read(f, buf, 64);
        vfs_close(f);

        if (strcmp(buf, expected) != 0) {
            printf("vfs_multiple_files: FAIL %s='%s' exp='%s'\n", path, buf, expected);
            return -1;
        }
    }

    return 0;
}

void run_vfs_tests(void) {
    int passed = 0;
    int failed = 0;

    if (test_vfs_basic() == 0) {
        printf("vfs_basic: PASS\n");
        passed++;
    } else {
        failed++;
    }

    if (test_vfs_directories() == 0) {
        printf("vfs_directories: PASS\n");
        passed++;
    } else {
        failed++;
    }

    if (test_vfs_seek() == 0) {
        printf("vfs_seek: PASS\n");
        passed++;
    } else {
        failed++;
    }

    if (test_vfs_multiple_files() == 0) {
        printf("vfs_multiple_files: PASS\n");
        passed++;
    } else {
        failed++;
    }

    printf("VFS: %d/%d passed\n", passed, passed + failed);
}
