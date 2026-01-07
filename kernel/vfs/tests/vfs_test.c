//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "../../kalloc/kalloc.h"
#include "../../lib/include/string.h"
#include "../../lib/include/logging.h"
#include "../../tty/tty.h"
#include "../../serial/serial.h"
#include "../vfs.h"

#define TEST_BUFSIZE 100

static int test_vfs_basic(void)
{
    struct file *f;
    int ret;
    const char *data = "Hello, tmpfs!";
    char buf[TEST_BUFSIZE];
    int64_t written, read_bytes;

    // Test 1: Create and open a file
    ret = vfs_open("/test.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_basic: FAIL vfs_open=%d", ret);
        return -1;
    }

    // Test 2: Write to file
    written = vfs_write(f, data, strlen(data));
    if (written != (int64_t) strlen(data))
    {
        LOG_SERIAL("VFS_TEST", "vfs_basic: FAIL write=%d exp=%d", written, strlen(data));
        vfs_close(f);
        return -1;
    }

    // Test 3: Close and reopen file
    vfs_close(f);
    ret = vfs_open("/test.txt", O_RDONLY, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_basic: FAIL reopen=%d", ret);
        return -1;
    }

    // Test 4: Read from file
    memset(buf, 0, TEST_BUFSIZE);
    read_bytes = vfs_read(f, buf, TEST_BUFSIZE);
    LOG_SERIAL("VFS_TEST", "[INFO] vfs_basic: read %d bytes: '%s'", (int) read_bytes, buf);

    if (read_bytes != (int64_t) strlen(data))
    {
        LOG_SERIAL("VFS_TEST", "vfs_basic: FAIL read=%d exp=%d", read_bytes, strlen(data));
        vfs_close(f);
        return -1;
    }
    if (strcmp(buf, data) != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_basic: FAIL data mismatch");
        vfs_close(f);
        return -1;
    }

    vfs_close(f);
    return 0;
}

static int test_vfs_directories(void)
{
    struct dentry *root;
    struct inode *root_inode;
    struct file *f;
    int ret;
    const char *data = "File in directory!";
    char buf[TEST_BUFSIZE];
    int64_t written, read_bytes;

    // Test 1: Create directory
    root = vfs_get_root();
    if (!root)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL no root");
        return -1;
    }

    root_inode = root->inode;
    if (!root_inode || !root_inode->i_op || !root_inode->i_op->mkdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL no mkdir op");
        return -1;
    }

    ret = root_inode->i_op->mkdir(root_inode, "testdir");
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL mkdir=%d", ret);
        return -1;
    }

    // Test 2: Create file in directory
    ret = vfs_open("/testdir/file.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL create=%d", ret);
        return -1;
    }

    // Test 3: Write and read from file in directory
    written = vfs_write(f, data, strlen(data));
    if (written != (int64_t) strlen(data))
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL write");
        vfs_close(f);
        return -1;
    }

    vfs_close(f);

    // Reopen and read
    ret = vfs_open("/testdir/file.txt", O_RDONLY, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL reopen=%d", ret);
        return -1;
    }

    memset(buf, 0, TEST_BUFSIZE);
    read_bytes = vfs_read(f, buf, TEST_BUFSIZE);
    if (read_bytes != (int64_t) strlen(data) || strcmp(buf, data) != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: FAIL verify");
        vfs_close(f);
        return -1;
    }

    vfs_close(f);
    return 0;
}

static int test_vfs_seek(void)
{
    struct file *f;
    int ret;
    const char *data = "0123456789ABCDEF";
    char buf[10];
    int64_t new_pos;

    // Create and write to file
    ret = vfs_open("/seektest.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_seek: FAIL create=%d", ret);
        return -1;
    }
    vfs_write(f, data, strlen(data));

    // Test SEEK_SET
    new_pos = vfs_lseek(f, 5, SEEK_SET);
    if (new_pos != 5)
    {
        LOG_SERIAL("VFS_TEST", "vfs_seek: FAIL SET pos=%d", new_pos);
        vfs_close(f);
        return -1;
    }
    memset(buf, 0, 10);
    vfs_read(f, buf, 5);
    if (strcmp(buf, "56789") != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_seek: FAIL SET data='%s'", buf);
        vfs_close(f);
        return -1;
    }

    // Test SEEK_CUR
    vfs_lseek(f, -3, SEEK_CUR);
    memset(buf, 0, 10);
    vfs_read(f, buf, 3);
    if (strcmp(buf, "789") != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_seek: FAIL CUR data='%s'", buf);
        vfs_close(f);
        return -1;
    }

    // Test SEEK_END
    vfs_lseek(f, -4, SEEK_END);
    memset(buf, 0, 10);
    vfs_read(f, buf, 4);
    if (strcmp(buf, "CDEF") != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_seek: FAIL END data='%s'", buf);
        vfs_close(f);
        return -1;
    }

    vfs_close(f);
    return 0;
}

static int test_vfs_multiple_files(void)
{
    int i;
    char path[32];
    char data[64];
    char expected[64];
    char buf[64];
    struct file *f;
    int ret;

    // Create multiple files
    for (i = 0; i < 5; i++)
    {
        // Will be "/file0.txt", "/file1.txt", etc
        strcpy(path, "/file0.txt");
        path[5] = '0' + i;

        ret = vfs_open(path, O_CREAT | O_RDWR, &f);
        if (ret != VFS_OK)
        {
            LOG_SERIAL("VFS_TEST", "vfs_multiple_files: FAIL create %s=%d", path, ret);
            return -1;
        }

        // Will be "File 0", "File 1", etc
        strcpy(data, "File 0");
        data[5] = '0' + i;

        vfs_write(f, data, strlen(data));
        vfs_close(f);
    }

    // Verify all files
    for (i = 0; i < 5; i++)
    {
        strcpy(path, "/file0.txt");
        path[5] = '0' + i;
        strcpy(expected, "File 0");
        expected[5] = '0' + i;

        ret = vfs_open(path, O_RDONLY, &f);
        if (ret != VFS_OK)
        {
            LOG_SERIAL("VFS_TEST", "vfs_multiple_files: FAIL open %s=%d", path, ret);
            return -1;
        }

        memset(buf, 0, 64);
        vfs_read(f, buf, 64);
        vfs_close(f);

        if (strcmp(buf, expected) != 0)
        {
            LOG_SERIAL("VFS_TEST", "vfs_multiple_files: FAIL %s='%s' exp='%s'", path, buf, expected);
            return -1;
        }
    }

    return 0;
}

static int test_vfs_readdir(void)
{
    struct file *dir_file;
    struct file *f;
    struct dirent dirents[10];
    int ret;
    int i;
    int found_files[5] = {0, 0, 0, 0, 0};
    int found_dirs[2] = {0, 0};
    const char *test_files[] = {"file0.txt", "file1.txt", "file2.txt", "file3.txt", "file4.txt"};
    const char *test_dirs[] = {"dir0", "dir1"};

    // Create test directory
    struct dentry *root = vfs_get_root();
    if (!root || !root->inode || !root->inode->i_op || !root->inode->i_op->mkdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL no mkdir op");
        return -1;
    }

    ret = root->inode->i_op->mkdir(root->inode, "readdirtest");
    if (ret != VFS_OK && ret != VFS_EEXIST)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL mkdir=%d", ret);
        return -1;
    }

    // Create multiple files in the test directory
    for (i = 0; i < 5; i++)
    {
        char path[64];
        strcpy(path, "/readdirtest/");
        strcpy(path + strlen(path), test_files[i]);
        ret = vfs_open(path, O_CREAT | O_RDWR, &f);
        if (ret != VFS_OK)
        {
            LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL create file %s=%d", path, ret);
            return -1;
        }
        vfs_close(f);
    }

    // Create subdirectories
    struct dentry *testdir = vfs_path_lookup("/readdirtest");
    if (!testdir || !testdir->inode || !testdir->inode->i_op || !testdir->inode->i_op->mkdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL no testdir");
        return -1;
    }

    for (i = 0; i < 2; i++)
    {
        ret = testdir->inode->i_op->mkdir(testdir->inode, test_dirs[i]);
        if (ret != VFS_OK)
        {
            LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL mkdir %s=%d", test_dirs[i], ret);
            return -1;
        }
    }

    // Open directory for reading
    ret = vfs_opendir("/readdirtest", &dir_file);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL open dir=%d", ret);
        return -1;
    }

    // Read directory entries
    int entries_read = vfs_readdir(dir_file, dirents, 10);
    if (entries_read < 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL readdir=%d", entries_read);
        vfs_close(dir_file);
        return -1;
    }

    LOG_SERIAL("VFS_TEST", "[INFO] vfs_readdir: read %d entries", entries_read);

    // Check that we found all expected entries
    for (i = 0; i < entries_read; i++)
    {
        LOG_SERIAL("VFS_TEST", "[INFO] Found entry: '%s' (type=%d, ino=%d)",
                   dirents[i].d_name, (int) dirents[i].d_type, (int) dirents[i].d_ino);

        // Check files
        for (int j = 0; j < 5; j++)
        {
            if (strcmp(dirents[i].d_name, test_files[j]) == 0)
            {
                if (dirents[i].d_type != INODE_TYPE_FILE)
                {
                    LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL %s wrong type (got %d, expected FILE)",
                               test_files[j], dirents[i].d_type);
                    vfs_close(dir_file);
                    return -1;
                }
                found_files[j] = 1;
                break;
            }
        }

        // Check directories
        for (int j = 0; j < 2; j++)
        {
            if (strcmp(dirents[i].d_name, test_dirs[j]) == 0)
            {
                if (dirents[i].d_type != INODE_TYPE_DIR)
                {
                    LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL %s wrong type (got %d, expected DIR)",
                               test_dirs[j], dirents[i].d_type);
                    vfs_close(dir_file);
                    return -1;
                }
                found_dirs[j] = 1;
                break;
            }
        }
    }

    vfs_close(dir_file);

    // Verify all files were found
    for (i = 0; i < 5; i++)
    {
        if (!found_files[i])
        {
            LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL file %s not found", test_files[i]);
            return -1;
        }
    }

    // Verify all directories were found
    for (i = 0; i < 2; i++)
    {
        if (!found_dirs[i])
        {
            LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL dir %s not found", test_dirs[i]);
            return -1;
        }
    }

    // Verify we have at least 7 entries (5 files + 2 dirs, possibly more like "." and "..")
    if (entries_read < 7)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: FAIL too few entries (got %d, expected at least 7)", entries_read);
        return -1;
    }

    return 0;
}

static int test_vfs_unlink(void)
{
    struct file *f;
    struct dentry *root;
    struct inode *root_inode;
    int ret;

    // Test 1: Delete a file
    // Create a file
    ret = vfs_open("/unlink_test_file.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL create file=%d", ret);
        return -1;
    }
    vfs_write(f, "test data", 9);
    vfs_close(f);

    // Verify file exists
    ret = vfs_open("/unlink_test_file.txt", O_RDONLY, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL file doesn't exist before unlink=%d", ret);
        return -1;
    }
    vfs_close(f);

    // Delete the file
    ret = vfs_unlink("/unlink_test_file.txt");
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL unlink file=%d", ret);
        return -1;
    }

    // Verify file no longer exists
    ret = vfs_open("/unlink_test_file.txt", O_RDONLY, &f);
    if (ret == VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL file still exists after unlink");
        vfs_close(f);
        return -1;
    }
    if (ret != VFS_ENOENT)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL wrong error after unlink (got %d, expected VFS_ENOENT)", ret);
        return -1;
    }

    // Test 2: Delete an empty directory
    root = vfs_get_root();
    if (!root || !root->inode || !root->inode->i_op || !root->inode->i_op->mkdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL no mkdir op");
        return -1;
    }

    root_inode = root->inode;
    ret = root_inode->i_op->mkdir(root_inode, "unlink_test_dir");
    if (ret != VFS_OK && ret != VFS_EEXIST)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL mkdir=%d", ret);
        return -1;
    }

    // Verify directory exists
    struct dentry *testdir = vfs_path_lookup("/unlink_test_dir");
    if (!testdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL directory doesn't exist before unlink");
        return -1;
    }
    vfs_put_dentry(testdir);

    // Delete the empty directory
    ret = vfs_unlink("/unlink_test_dir");
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL unlink empty dir=%d", ret);
        return -1;
    }

    // Verify directory no longer exists
    testdir = vfs_path_lookup("/unlink_test_dir");
    if (testdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL directory still exists after unlink");
        vfs_put_dentry(testdir);
        return -1;
    }

    // Test 3: Try to delete a non-empty directory (should fail)
    // Create directory
    ret = root_inode->i_op->mkdir(root_inode, "unlink_test_dir2");
    if (ret != VFS_OK && ret != VFS_EEXIST)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL mkdir dir2=%d", ret);
        return -1;
    }

    // Create a file inside the directory
    ret = vfs_open("/unlink_test_dir2/file.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL create file in dir2=%d", ret);
        return -1;
    }
    vfs_close(f);

    // Try to delete non-empty directory (should fail)
    ret = vfs_unlink("/unlink_test_dir2");
    if (ret != VFS_ENOTEMPTY)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL unlink non-empty dir (got %d, expected VFS_ENOTEMPTY)", ret);
        return -1;
    }

    // Verify directory still exists
    testdir = vfs_path_lookup("/unlink_test_dir2");
    if (!testdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL directory was deleted when it shouldn't be");
        return -1;
    }
    vfs_put_dentry(testdir);

    // Clean up: delete the file first, then the directory
    ret = vfs_unlink("/unlink_test_dir2/file.txt");
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL cleanup file=%d", ret);
        return -1;
    }

    ret = vfs_unlink("/unlink_test_dir2");
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL cleanup dir2=%d", ret);
        return -1;
    }

    // Test 4: Try to delete non-existent file (should fail)
    ret = vfs_unlink("/nonexistent_file.txt");
    if (ret != VFS_ENOENT)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: FAIL unlink nonexistent (got %d, expected VFS_ENOENT)", ret);
        return -1;
    }

    return 0;
}

static int test_vfs_mount_at(void)
{
    struct file *f;
    struct file *dir_file;
    struct dirent dirent_buf[10];
    int ret;
    const char *data = "Test data in mounted filesystem!";
    char buf[TEST_BUFSIZE];
    int64_t written, read_bytes;
    int entries_read;

    // Test 1: Verify that /mnt/test exists and is a directory
    struct dentry *mount_point = vfs_path_lookup("/mnt/test");
    if (!mount_point)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL mount point /mnt/test doesn't exist");
        return -1;
    }

    if (!mount_point->inode || mount_point->inode->type != INODE_TYPE_DIR)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL /mnt/test is not a directory");
        vfs_put_dentry(mount_point);
        return -1;
    }
    vfs_put_dentry(mount_point);

    // Test 2: Create a file in the mounted filesystem
    ret = vfs_open("/mnt/test/testfile.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL create file=%d", ret);
        return -1;
    }

    // Test 3: Write to file
    written = vfs_write(f, data, strlen(data));
    if (written != (int64_t) strlen(data))
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL write=%d exp=%d", (int) written, (int) strlen(data));
        vfs_close(f);
        return -1;
    }
    vfs_close(f);

    // Test 4: Read from file
    ret = vfs_open("/mnt/test/testfile.txt", O_RDONLY, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL reopen=%d", ret);
        return -1;
    }

    memset(buf, 0, TEST_BUFSIZE);
    read_bytes = vfs_read(f, buf, TEST_BUFSIZE);
    if (read_bytes != (int64_t) strlen(data))
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL read=%d exp=%d", (int) read_bytes, (int) strlen(data));
        vfs_close(f);
        return -1;
    }
    if (strcmp(buf, data) != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL data mismatch: got '%s', expected '%s'", buf, data);
        vfs_close(f);
        return -1;
    }
    vfs_close(f);

    // Test 5: Create a subdirectory in the mounted filesystem
    mount_point = vfs_path_lookup("/mnt/test");
    if (!mount_point || !mount_point->inode || !mount_point->inode->i_op || !mount_point->inode->i_op->mkdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL no mkdir op");
        return -1;
    }

    ret = mount_point->inode->i_op->mkdir(mount_point->inode, "subdir");
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL mkdir subdir=%d", ret);
        vfs_put_dentry(mount_point);
        return -1;
    }
    vfs_put_dentry(mount_point);

    // Test 6: Verify subdirectory exists
    struct dentry *subdir = vfs_path_lookup("/mnt/test/subdir");
    if (!subdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL subdirectory doesn't exist");
        return -1;
    }
    if (!subdir->inode || subdir->inode->type != INODE_TYPE_DIR)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL subdirectory is not a directory");
        vfs_put_dentry(subdir);
        return -1;
    }
    vfs_put_dentry(subdir);

    // Test 7: Create a file in the subdirectory
    ret = vfs_open("/mnt/test/subdir/subfile.txt", O_CREAT | O_RDWR, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL create file in subdir=%d", ret);
        return -1;
    }

    const char *subdata = "Data in subdirectory";
    written = vfs_write(f, subdata, strlen(subdata));
    if (written != (int64_t) strlen(subdata))
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL write to subfile=%d exp=%d", (int) written, (int) strlen(subdata));
        vfs_close(f);
        return -1;
    }
    vfs_close(f);

    // Test 8: Read from file in subdirectory
    ret = vfs_open("/mnt/test/subdir/subfile.txt", O_RDONLY, &f);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL open subfile=%d", ret);
        return -1;
    }

    memset(buf, 0, TEST_BUFSIZE);
    read_bytes = vfs_read(f, buf, TEST_BUFSIZE);
    if (read_bytes != (int64_t) strlen(subdata) || strcmp(buf, subdata) != 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL read subfile: got '%s', expected '%s'", buf, subdata);
        vfs_close(f);
        return -1;
    }
    vfs_close(f);

    // Test 9: List directory contents using readdir
    ret = vfs_opendir("/mnt/test", &dir_file);
    if (ret != VFS_OK)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL opendir=%d", ret);
        return -1;
    }

    entries_read = vfs_readdir(dir_file, dirent_buf, 10);
    if (entries_read < 2)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL readdir returned %d entries, expected at least 2", entries_read);
        vfs_close(dir_file);
        return -1;
    }

    // Verify that we can find testfile.txt and subdir in the listing
    int found_file = 0;
    int found_subdir = 0;
    for (int i = 0; i < entries_read; i++)
    {
        if (strcmp(dirent_buf[i].d_name, "testfile.txt") == 0)
        {
            found_file = 1;
        }
        if (strcmp(dirent_buf[i].d_name, "subdir") == 0)
        {
            found_subdir = 1;
        }
    }

    if (!found_file)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL testfile.txt not found in directory listing");
        vfs_close(dir_file);
        return -1;
    }
    if (!found_subdir)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: FAIL subdir not found in directory listing");
        vfs_close(dir_file);
        return -1;
    }

    vfs_close(dir_file);
    return 0;
}

void run_vfs_tests(void)
{
    int passed = 0;
    int failed = 0;

    if (test_vfs_basic() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_basic: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    if (test_vfs_directories() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_directories: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    if (test_vfs_seek() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_seek: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    if (test_vfs_multiple_files() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_multiple_files: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    if (test_vfs_readdir() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_readdir: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    if (test_vfs_unlink() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_unlink: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    if (test_vfs_mount_at() == 0)
    {
        LOG_SERIAL("VFS_TEST", "vfs_mount_at: PASS");
        passed++;
    }
    else
    {
        failed++;
    }

    LOG_SERIAL("VFS_TEST", "VFS: %d/%d passed", passed, passed + failed);
}
