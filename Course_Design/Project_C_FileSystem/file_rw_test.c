/**
 * File Read/Write Test
 * file_rw_test.c
 * 
 * 测试文件读写功能的完整性和正确性
 */

#include "file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_DISK_FILE "test_file_rw.img"
#define TEST_DISK_SIZE (8 * 1024 * 1024)

int main(void) {
    printf("================ 文件读写功能测试程序 ================\n");
    
    // 清理旧文件
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
    
    // 关闭磁盘并重新打开以确保状态同步
    disk_close();
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("错误：重新初始化磁盘失败\n");
        return 1;
    }
    
    // 2. 创建测试文件
    printf("\n步骤 2: 创建测试文件...\n");
    result = fs_create("test_file.txt");
    if (result != FS_SUCCESS) {
        printf("错误：创建文件失败\n");
        return 1;
    }
    printf("测试文件创建成功\n");
    
    // 3. 打开文件进行读写
    printf("\n步骤 3: 打开文件...\n");
    int fd = fs_open("test_file.txt");
    if (fd < 0) {
        printf("错误：打开文件失败\n");
        return 1;
    }
    printf("文件已打开，fd=%d\n", fd);
    
    // 4. 测试文件写入
    printf("\n步骤 4: 测试文件写入...\n");
    
    const char* test_data1 = "Hello, World! This is a test.";
    int bytes_written = fs_write(fd, test_data1, strlen(test_data1));
    printf("写入结果: %d 字节 (期望: %zu)\n", bytes_written, strlen(test_data1));
    
    // 获取文件当前位置和大小
    int current_pos = fs_tell(fd);
    int file_size = fs_size(fd);
    printf("当前位置: %d, 文件大小: %d\n", current_pos, file_size);
    
    // 5. 写入更多数据
    printf("\n步骤 5: 追加写入数据...\n");
    
    const char* test_data2 = "\nSecond line of text.";
    bytes_written = fs_write(fd, test_data2, strlen(test_data2));
    printf("追加写入结果: %d 字节 (期望: %zu)\n", bytes_written, strlen(test_data2));
    
    current_pos = fs_tell(fd);
    file_size = fs_size(fd);
    printf("当前位置: %d, 文件大小: %d\n", current_pos, file_size);
    
    // 6. 测试文件定位
    printf("\n步骤 6: 测试文件定位...\n");
    
    // 回到文件开头
    int new_pos = fs_seek(fd, 0, SEEK_SET);
    printf("定位到开头: %d\n", new_pos);
    
    // 7. 测试文件读取
    printf("\n步骤 7: 测试文件读取...\n");
    
    char read_buffer[200];
    memset(read_buffer, 0, sizeof(read_buffer));
    
    int bytes_read = fs_read(fd, read_buffer, sizeof(read_buffer) - 1);
    printf("读取结果: %d 字节\n", bytes_read);
    printf("读取内容: [%s]\n", read_buffer);
    
    // 8. 测试部分读取
    printf("\n步骤 8: 测试部分读取...\n");
    
    // 定位到文件中间
    fs_seek(fd, 7, SEEK_SET);
    
    char partial_buffer[20];
    memset(partial_buffer, 0, sizeof(partial_buffer));
    
    bytes_read = fs_read(fd, partial_buffer, 10);
    printf("部分读取结果: %d 字节\n", bytes_read);
    printf("部分读取内容: [%s]\n", partial_buffer);
    
    // 9. 测试跨块写入（写入大量数据）
    printf("\n步骤 9: 测试跨块写入...\n");
    
    // 定位到文件末尾
    fs_seek(fd, 0, SEEK_END);
    
    // 准备大块数据（超过1024字节）
    char large_data[1500];
    for (int i = 0; i < sizeof(large_data) - 1; i++) {
        large_data[i] = 'A' + (i % 26);
    }
    large_data[sizeof(large_data) - 1] = '\0';
    
    bytes_written = fs_write(fd, large_data, strlen(large_data));
    printf("大块写入结果: %d 字节 (期望: %zu)\n", bytes_written, strlen(large_data));
    
    file_size = fs_size(fd);
    printf("写入后文件大小: %d\n", file_size);
    
    // 10. 验证跨块读取
    printf("\n步骤 10: 验证跨块读取...\n");
    
    // 定位到大块数据开始处
    int large_data_start = strlen(test_data1) + strlen(test_data2);
    fs_seek(fd, large_data_start, SEEK_SET);
    
    char verify_buffer[1600];
    memset(verify_buffer, 0, sizeof(verify_buffer));
    
    bytes_read = fs_read(fd, verify_buffer, sizeof(verify_buffer) - 1);
    printf("跨块读取结果: %d 字节\n", bytes_read);
    
    // 验证数据正确性
    int match = (strncmp(verify_buffer, large_data, strlen(large_data)) == 0);
    printf("数据验证: %s\n", match ? "通过" : "失败");
    
    if (!match) {
        printf("期望: %.50s...\n", large_data);
        printf("实际: %.50s...\n", verify_buffer);
    }
    
    // 11. 测试文件末尾读取
    printf("\n步骤 11: 测试文件末尾读取...\n");
    
    fs_seek(fd, 0, SEEK_END);
    char eof_buffer[10];
    bytes_read = fs_read(fd, eof_buffer, sizeof(eof_buffer));
    printf("文件末尾读取: %d 字节 (应该为0)\n", bytes_read);
    
    // 12. 测试错误情况
    printf("\n步骤 12: 测试错误情况...\n");
    
    // 无效的文件描述符
    int invalid_result = fs_read(999, read_buffer, 10);
    printf("无效fd读取: %d (应该为负数)\n", invalid_result);
    
    invalid_result = fs_write(999, "test", 4);
    printf("无效fd写入: %d (应该为负数)\n", invalid_result);
    
    // NULL指针
    invalid_result = fs_read(fd, NULL, 10);
    printf("NULL缓冲区读取: %d (应该为负数)\n", invalid_result);
    
    invalid_result = fs_write(fd, NULL, 10);
    printf("NULL数据写入: %d (应该为负数)\n", invalid_result);
    
    // 13. 关闭文件
    printf("\n步骤 13: 关闭文件...\n");
    fs_close(fd);
    printf("文件已关闭\n");
    
    // 14. 重新打开文件验证持久性
    printf("\n步骤 14: 验证数据持久性...\n");
    
    fd = fs_open("test_file.txt");
    if (fd < 0) {
        printf("错误：重新打开文件失败\n");
        return 1;
    }
    
    file_size = fs_size(fd);
    printf("重新打开后文件大小: %d\n", file_size);
    
    // 读取并验证第一行数据
    char persist_buffer[100];
    memset(persist_buffer, 0, sizeof(persist_buffer));
    fs_read(fd, persist_buffer, strlen(test_data1));
    
    int persist_match = (strncmp(persist_buffer, test_data1, strlen(test_data1)) == 0);
    printf("数据持久性验证: %s\n", persist_match ? "通过" : "失败");
    
    fs_close(fd);
    
    // 15. 清理资源
    printf("\n步骤 15: 清理资源...\n");
    disk_close();
    unlink(TEST_DISK_FILE);
    
    printf("\n================ 测试完成 ================\n");
    printf("文件读写功能测试完成！\n");
    
    return 0;
} 