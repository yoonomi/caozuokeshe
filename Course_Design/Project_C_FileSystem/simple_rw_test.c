/**
 * Simple File Read/Write Test
 * simple_rw_test.c
 * 
 * 基于演示程序框架的简单文件读写测试
 */

#include "file_ops.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_DISK_FILE "simple_rw_test.img"
#define TEST_DISK_SIZE (8 * 1024 * 1024)

int main(void) {
    printf("================ 简单文件读写测试 ================\n");
    
    // 清理
    unlink(TEST_DISK_FILE);
    
    // 初始化并格式化
    printf("初始化文件系统...\n");
    int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("磁盘初始化失败\n");
        return 1;
    }
    
    format_disk();
    
    // 关闭并重新打开
    disk_close();
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("重新打开失败\n");
        return 1;
    }
    
    // 创建文件（使用原有的创建函数）
    printf("创建文件...\n");
    result = fs_create("test.txt");
    printf("创建结果: %s\n", result == FS_SUCCESS ? "成功" : "失败");
    
    if (result != FS_SUCCESS) {
        printf("文件创建失败，退出测试\n");
        disk_close();
        return 1;
    }
    
    // 打开文件
    printf("打开文件...\n");
    int fd = fs_open("test.txt");
    printf("打开结果: fd=%d\n", fd);
    
    if (fd < 0) {
        printf("文件打开失败，退出测试\n");
        disk_close();
        return 1;
    }
    
    // 测试写入
    printf("\n测试文件写入...\n");
    const char* data = "Hello, World!";
    int bytes_written = fs_write(fd, data, strlen(data));
    printf("写入结果: %d 字节 (期望: %zu)\n", bytes_written, strlen(data));
    
    // 获取文件信息
    int file_size = fs_size(fd);
    int position = fs_tell(fd);
    printf("文件大小: %d, 当前位置: %d\n", file_size, position);
    
    // 定位到开头
    printf("\n定位到文件开头...\n");
    int seek_result = fs_seek(fd, 0, SEEK_SET);
    printf("定位结果: %d\n", seek_result);
    
    // 测试读取
    printf("\n测试文件读取...\n");
    char buffer[50];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read = fs_read(fd, buffer, sizeof(buffer) - 1);
    printf("读取结果: %d 字节\n", bytes_read);
    printf("读取内容: [%s]\n", buffer);
    
    // 验证数据
    int data_match = (strcmp(buffer, data) == 0);
    printf("数据验证: %s\n", data_match ? "通过" : "失败");
    
    // 关闭文件
    printf("\n关闭文件...\n");
    fs_close(fd);
    
    // 清理
    disk_close();
    unlink(TEST_DISK_FILE);
    
    printf("\n测试完成！\n");
    return 0;
} 