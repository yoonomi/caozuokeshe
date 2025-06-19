/**
 * Comprehensive File Read/Write Test
 * comprehensive_rw_test.c
 * 
 * 全面测试文件读写功能的正确性和性能
 */

#include "file_ops.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_DISK_FILE "comprehensive_rw_test.img"
#define TEST_DISK_SIZE (16 * 1024 * 1024)

void test_basic_operations(void);
void test_large_file_operations(void);
void test_multiple_files(void);
void test_seek_operations(void);
void test_error_conditions(void);

int main(void) {
    printf("================ 全面文件读写功能测试 ================\n");
    
    // 清理并初始化
    unlink(TEST_DISK_FILE);
    
    printf("初始化文件系统...\n");
    int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("磁盘初始化失败\n");
        return 1;
    }
    
    format_disk();
    
    // 关闭并重新打开以确保状态同步
    disk_close();
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("重新打开磁盘失败\n");
        return 1;
    }
    
    printf("文件系统初始化完成\n\n");
    
    // 运行各项测试
    test_basic_operations();
    test_large_file_operations();
    test_multiple_files();
    test_seek_operations();
    test_error_conditions();
    
    // 清理
    disk_close();
    unlink(TEST_DISK_FILE);
    
    printf("\n================ 全面测试完成 ================\n");
    return 0;
}

void test_basic_operations(void) {
    printf("=== 测试 1: 基本读写操作 ===\n");
    
    // 创建文件
    int result = fs_create("basic_test.txt");
    printf("创建文件: %s\n", result == FS_SUCCESS ? "成功" : "失败");
    
    // 打开文件
    int fd = fs_open("basic_test.txt");
    printf("打开文件: fd=%d\n", fd);
    
    if (fd >= 0) {
        // 测试写入
        const char* data = "Basic file operations test";
        int bytes_written = fs_write(fd, data, strlen(data));
        printf("写入: %d/%zu 字节\n", bytes_written, strlen(data));
        
        // 检查文件大小
        int size = fs_size(fd);
        printf("文件大小: %d 字节\n", size);
        
        // 回到开头读取
        fs_seek(fd, 0, SEEK_SET);
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = fs_read(fd, buffer, sizeof(buffer) - 1);
        printf("读取: %d 字节\n", bytes_read);
        printf("内容: [%s]\n", buffer);
        
        // 验证
        int match = (strcmp(buffer, data) == 0);
        printf("验证: %s\n", match ? "通过" : "失败");
        
        fs_close(fd);
    }
    
    printf("基本操作测试完成\n\n");
}

void test_large_file_operations(void) {
    printf("=== 测试 2: 大文件读写操作 ===\n");
    
    int result = fs_create("large_test.txt");
    printf("创建大文件: %s\n", result == FS_SUCCESS ? "成功" : "失败");
    
    int fd = fs_open("large_test.txt");
    if (fd >= 0) {
        // 创建大块数据 (跨越多个块)
        char large_data[2048];
        for (int i = 0; i < sizeof(large_data) - 1; i++) {
            large_data[i] = 'A' + (i % 26);
        }
        large_data[sizeof(large_data) - 1] = '\0';
        
        printf("写入大数据块 (%zu 字节)...\n", strlen(large_data));
        int bytes_written = fs_write(fd, large_data, strlen(large_data));
        printf("写入结果: %d/%zu 字节\n", bytes_written, strlen(large_data));
        
        int file_size = fs_size(fd);
        printf("文件大小: %d 字节\n", file_size);
        
        // 读取并验证
        fs_seek(fd, 0, SEEK_SET);
        char* read_buffer = malloc(sizeof(large_data));
        memset(read_buffer, 0, sizeof(large_data));
        
        int bytes_read = fs_read(fd, read_buffer, sizeof(large_data) - 1);
        printf("读取结果: %d 字节\n", bytes_read);
        
        int match = (strncmp(read_buffer, large_data, strlen(large_data)) == 0);
        printf("大文件验证: %s\n", match ? "通过" : "失败");
        
        free(read_buffer);
        fs_close(fd);
    }
    
    printf("大文件操作测试完成\n\n");
}

void test_multiple_files(void) {
    printf("=== 测试 3: 多文件操作 ===\n");
    
    // 创建多个文件
    const char* filenames[] = {"file1.txt", "file2.txt", "file3.txt"};
    int fds[3];
    
    for (int i = 0; i < 3; i++) {
        fs_create(filenames[i]);
        fds[i] = fs_open(filenames[i]);
        printf("创建并打开 %s: fd=%d\n", filenames[i], fds[i]);
    }
    
    // 向每个文件写入不同数据
    for (int i = 0; i < 3; i++) {
        if (fds[i] >= 0) {
            char data[50];
            snprintf(data, sizeof(data), "Content of file %d", i + 1);
            int bytes = fs_write(fds[i], data, strlen(data));
            printf("文件%d写入: %d 字节\n", i + 1, bytes);
        }
    }
    
    // 读取并验证每个文件
    for (int i = 0; i < 3; i++) {
        if (fds[i] >= 0) {
            fs_seek(fds[i], 0, SEEK_SET);
            char buffer[50];
            memset(buffer, 0, sizeof(buffer));
            int bytes = fs_read(fds[i], buffer, sizeof(buffer) - 1);
            printf("文件%d读取: %d 字节, 内容: [%s]\n", i + 1, bytes, buffer);
            fs_close(fds[i]);
        }
    }
    
    printf("多文件操作测试完成\n\n");
}

void test_seek_operations(void) {
    printf("=== 测试 4: 文件定位操作 ===\n");
    
    fs_create("seek_test.txt");
    int fd = fs_open("seek_test.txt");
    
    if (fd >= 0) {
        // 写入测试数据
        const char* data = "0123456789ABCDEFGHIJ";
        fs_write(fd, data, strlen(data));
        printf("写入测试数据: %s\n", data);
        
        // 测试不同的seek操作
        printf("文件大小: %d\n", fs_size(fd));
        
        // SEEK_SET测试
        fs_seek(fd, 5, SEEK_SET);
        printf("SEEK_SET(5), 位置: %d\n", fs_tell(fd));
        
        char buffer[10];
        memset(buffer, 0, sizeof(buffer));
        fs_read(fd, buffer, 5);
        printf("从位置5读取5字节: [%s]\n", buffer);
        
        // SEEK_CUR测试
        fs_seek(fd, 2, SEEK_CUR);
        printf("SEEK_CUR(2), 位置: %d\n", fs_tell(fd));
        
        memset(buffer, 0, sizeof(buffer));
        fs_read(fd, buffer, 3);
        printf("继续读取3字节: [%s]\n", buffer);
        
        // SEEK_END测试
        fs_seek(fd, -5, SEEK_END);
        printf("SEEK_END(-5), 位置: %d\n", fs_tell(fd));
        
        memset(buffer, 0, sizeof(buffer));
        fs_read(fd, buffer, 5);
        printf("从末尾读取5字节: [%s]\n", buffer);
        
        fs_close(fd);
    }
    
    printf("文件定位操作测试完成\n\n");
}

void test_error_conditions(void) {
    printf("=== 测试 5: 错误处理 ===\n");
    
    // 测试无效文件描述符
    int result = fs_read(999, NULL, 10);
    printf("无效fd读取: %d (应为负数)\n", result);
    
    result = fs_write(999, "test", 4);
    printf("无效fd写入: %d (应为负数)\n", result);
    
    // 创建一个文件用于测试
    fs_create("error_test.txt");
    int fd = fs_open("error_test.txt");
    
    if (fd >= 0) {
        // 测试NULL指针
        result = fs_read(fd, NULL, 10);
        printf("NULL缓冲区读取: %d (应为负数)\n", result);
        
        result = fs_write(fd, NULL, 10);
        printf("NULL数据写入: %d (应为负数)\n", result);
        
        // 测试0字节操作
        result = fs_read(fd, "buffer", 0);
        printf("0字节读取: %d (应为负数)\n", result);
        
        result = fs_write(fd, "data", 0);
        printf("0字节写入: %d (应为负数)\n", result);
        
        fs_close(fd);
    }
    
    // 测试无效seek参数
    fs_create("seek_error_test.txt");
    fd = fs_open("seek_error_test.txt");
    if (fd >= 0) {
        result = fs_seek(fd, -10, SEEK_SET);
        printf("无效SEEK_SET: %d (应为负数)\n", result);
        
        result = fs_seek(fd, 0, 999);  // 无效whence
        printf("无效whence: %d (应为负数)\n", result);
        
        fs_close(fd);
    }
    
    printf("错误处理测试完成\n\n");
} 