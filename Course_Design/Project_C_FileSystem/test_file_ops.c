/**
 * Simple File Operations Test
 */

#include "fs_ops.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_DISK_FILE "test_file_ops.img"
#define TEST_DISK_SIZE (8 * 1024 * 1024)

int main(void) {
    printf("================ 文件操作功能测试程序 ================\n");
    
    // 删除旧文件
    unlink(TEST_DISK_FILE);
    
    // 1. 初始化文件系统
    printf("\n步骤 1: 初始化文件系统...\n");
    int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("错误：磁盘初始化失败\n");
        return 1;
    }
    
    format_disk();
    printf("文件系统初始化完成\n");
    
    // 2. 测试文件创建
    printf("\n步骤 2: 测试文件创建...\n");
    
    result = fs_create("test1.txt");
    printf("创建 test1.txt: %s\n", result == FS_SUCCESS ? "成功" : "失败");
    
    result = fs_create("/test2.txt");
    printf("创建 /test2.txt: %s\n", result == FS_SUCCESS ? "成功" : "失败");
    
    // 3. 测试文件打开
    printf("\n步骤 3: 测试文件打开...\n");
    
    int fd1 = fs_open("test1.txt");
    printf("打开 test1.txt: fd=%d\n", fd1);
    
    int fd2 = fs_open("/test2.txt");
    printf("打开 /test2.txt: fd=%d\n", fd2);
    
    // 4. 测试文件关闭
    printf("\n步骤 4: 测试文件关闭...\n");
    
    if (fd1 >= 0) {
        fs_close(fd1);
    }
    if (fd2 >= 0) {
        fs_close(fd2);
    }
    
    // 清理
    disk_close();
    unlink(TEST_DISK_FILE);
    
    printf("\n测试完成！\n");
    return 0;
} 