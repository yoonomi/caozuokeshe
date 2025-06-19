/**
 * File System Format Demo
 * format_demo.c
 * 
 * 演示文件系统格式化功能的使用方法。
 * 展示如何创建和格式化一个新的文件系统。
 */

#include "fs_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEMO_DISK_FILE "demo_filesystem.img"
#define DEMO_DISK_SIZE (16 * 1024 * 1024)  // 16MB 演示磁盘

int main(void) {
    printf("================== 文件系统格式化演示程序 ==================\n");
    printf("演示文件: %s\n", DEMO_DISK_FILE);
    printf("磁盘大小: %d 字节 (%.1f MB)\n", DEMO_DISK_SIZE, DEMO_DISK_SIZE / (1024.0 * 1024.0));
    printf("===========================================================\n");
    
    // 删除可能存在的旧文件
    unlink(DEMO_DISK_FILE);
    
    printf("\n步骤 1: 初始化磁盘模拟器...\n");
    int result = disk_init(DEMO_DISK_FILE, DEMO_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("错误：磁盘初始化失败: %s\n", disk_error_to_string(result));
        return 1;
    }
    
    // 获取磁盘信息
    uint32_t total_blocks, block_size;
    uint64_t disk_size;
    result = disk_get_info(&total_blocks, &block_size, &disk_size);
    if (result == DISK_SUCCESS) {
        printf("磁盘已初始化:\n");
        printf("  总块数: %u\n", total_blocks);
        printf("  块大小: %u 字节\n", block_size);
        printf("  磁盘大小: %lu 字节\n", disk_size);
    }
    
    printf("\n步骤 2: 格式化文件系统...\n");
    format_disk();
    
    printf("\n步骤 3: 验证格式化结果...\n");
    
    // 读取并验证超级块
    fs_superblock_t sb;
    fs_error_t fs_result = fs_ops_read_superblock(&sb);
    if (fs_result == FS_SUCCESS) {
        printf("超级块验证成功:\n");
        printf("  文件系统魔数: 0x%x\n", sb.magic_number);
        printf("  版本: %u\n", sb.version);
        printf("  总inode数: %u (可用: %u)\n", sb.total_inodes, sb.free_inodes);
        printf("  数据块起始: %u\n", sb.data_blocks_start);
        printf("  根目录inode: %u\n", sb.root_inode);
        
        // 计算存储效率
        uint32_t used_blocks = total_blocks - sb.free_blocks;
        uint32_t used_inodes = sb.total_inodes - sb.free_inodes;
        
        printf("\n存储利用率:\n");
        printf("  已用块数: %u / %u (%.1f%%)\n", 
               used_blocks, total_blocks, 
               (used_blocks * 100.0) / total_blocks);
        printf("  已用inode: %u / %u (%.1f%%)\n", 
               used_inodes, sb.total_inodes, 
               (used_inodes * 100.0) / sb.total_inodes);
        printf("  可用空间: %.2f MB\n", 
               (sb.free_blocks * block_size) / (1024.0 * 1024.0));
    } else {
        printf("超级块验证失败: %s\n", fs_ops_error_to_string(fs_result));
    }
    
    printf("\n步骤 4: 验证根目录...\n");
    
    // 计算根目录inode在inode表中的位置
    uint32_t inodes_per_block = DISK_BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = sb.inode_table_start + (ROOT_INODE_NUM / inodes_per_block);
    uint32_t inode_offset = (ROOT_INODE_NUM % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取根目录inode
    char inode_block[DISK_BLOCK_SIZE];
    result = disk_read_block(inode_block_num, inode_block);
    if (result == DISK_SUCCESS) {
        fs_inode_t *root_inode = (fs_inode_t *)(inode_block + inode_offset);
        
        printf("根目录inode信息:\n");
        printf("  inode号: %u\n", root_inode->inode_number);
        printf("  文件类型: %u (目录)\n", root_inode->file_type);
        printf("  权限: 0%o\n", root_inode->permissions);
        printf("  所有者: UID=%u, GID=%u\n", root_inode->owner_uid, root_inode->owner_gid);
        printf("  大小: %lu 字节\n", root_inode->file_size);
        printf("  链接数: %u\n", root_inode->link_count);
        printf("  数据块: %u\n", root_inode->direct_blocks[0]);
        
        // 读取根目录内容
        char dir_block[DISK_BLOCK_SIZE];
        result = disk_read_block(root_inode->direct_blocks[0], dir_block);
        if (result == DISK_SUCCESS) {
            fs_dir_entry_t *entries = (fs_dir_entry_t *)dir_block;
            printf("\n根目录内容:\n");
            
            for (int i = 0; i < 2; i++) {
                if (entries[i].is_valid) {
                    printf("  [%d] \"%s\" -> inode %u (类型: %u)\n", 
                           i, entries[i].filename, entries[i].inode_number, entries[i].file_type);
                }
            }
        }
    }
    
    printf("\n步骤 5: 文件系统状态统计...\n");
    fs_ops_print_status();
    
    printf("\n步骤 6: 清理资源...\n");
    
    // 同步数据
    result = disk_sync();
    if (result == DISK_SUCCESS) {
        printf("数据已同步到磁盘\n");
    }
    
    // 关闭磁盘
    result = disk_close();
    if (result == DISK_SUCCESS) {
        printf("磁盘已关闭\n");
    }
    
    printf("\n步骤 7: 重新打开文件系统进行文件操作测试...\n");
    
    // 重新初始化磁盘进行文件操作测试（传入原始大小，但实际大小会从文件头读取）
    result = disk_init(DEMO_DISK_FILE, DEMO_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("错误：无法重新打开磁盘文件\n");
        return 1;
    }
    printf("磁盘文件已重新打开\n");
    
    printf("\n步骤 8: 测试文件操作功能...\n");
    
    // 测试文件创建
    printf("测试文件创建:\n");
    int result1 = fs_create("test1.txt");
    printf("  创建 test1.txt: %s\n", result1 == FS_SUCCESS ? "成功" : fs_ops_error_to_string(result1));
    
    int result2 = fs_create("/test2.txt");
    printf("  创建 /test2.txt: %s\n", result2 == FS_SUCCESS ? "成功" : fs_ops_error_to_string(result2));
    
    // 测试重复创建
    int result3 = fs_create("test1.txt");
    printf("  重复创建 test1.txt: %s\n", fs_ops_error_to_string(result3));
    
    // 测试文件打开
    printf("\n测试文件打开:\n");
    int fd1 = fs_open("test1.txt");
    printf("  打开 test1.txt: fd=%d\n", fd1);
    
    int fd2 = fs_open("/test2.txt");
    printf("  打开 /test2.txt: fd=%d\n", fd2);
    
    int fd3 = fs_open("nonexistent.txt");
    printf("  打开不存在文件: %s\n", fd3 < 0 ? fs_ops_error_to_string(fd3) : "意外成功");
    
    // 测试文件关闭
    printf("\n测试文件关闭:\n");
    if (fd1 >= 0) {
        fs_close(fd1);
        printf("  关闭 fd=%d\n", fd1);
    }
    if (fd2 >= 0) {
        fs_close(fd2);
        printf("  关闭 fd=%d\n", fd2);
    }
    
    // 测试无效关闭
    fs_close(-1);
    printf("  尝试关闭无效fd=-1\n");
    
    printf("\n步骤 9: 测试文件读写功能...\n");
    
    // 测试写入数据
    if (fd1 >= 0) {
        // 重新打开文件进行读写测试
        fd1 = fs_open("test1.txt");
        if (fd1 >= 0) {
            printf("  重新打开 test1.txt: fd=%d\n", fd1);
            
            // 写入数据
            const char* test_data = "Hello, File System!";
            int bytes_written = fs_write(fd1, test_data, strlen(test_data));
            printf("  写入数据: %d 字节 (期望: %zu)\n", bytes_written, strlen(test_data));
            
            // 获取文件信息
            int file_size = fs_size(fd1);
            int current_pos = fs_tell(fd1);
            printf("  文件大小: %d, 当前位置: %d\n", file_size, current_pos);
            
            // 定位到文件开头
            int seek_pos = fs_seek(fd1, 0, SEEK_SET);
            printf("  定位到开头: %d\n", seek_pos);
            
            // 读取数据
            char read_buffer[50];
            memset(read_buffer, 0, sizeof(read_buffer));
            int bytes_read = fs_read(fd1, read_buffer, sizeof(read_buffer) - 1);
            printf("  读取数据: %d 字节\n", bytes_read);
            printf("  读取内容: [%s]\n", read_buffer);
            
            // 验证数据
            int match = (strcmp(read_buffer, test_data) == 0);
            printf("  数据验证: %s\n", match ? "通过" : "失败");
            
            fs_close(fd1);
        }
    }

    printf("\n==================== 演示完成 ====================\n");
    printf("文件系统已成功格式化并保存到: %s\n", DEMO_DISK_FILE);
    printf("文件操作功能测试完成\n");
    printf("===================================================\n");
    
    return 0;
} 