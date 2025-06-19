/**
 * File System Format Test
 * fs_format_test.c
 * 
 * 测试文件系统格式化功能的完整性和正确性。
 * 
 * 测试内容：
 * - 磁盘初始化
 * - 文件系统格式化
 * - 超级块验证
 * - 位图验证
 * - 根目录验证
 */

#include "fs_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*==============================================================================
 * 测试常量和宏
 *============================================================================*/

#define TEST_DISK_FILE      "test_filesystem.img"
#define TEST_DISK_SIZE      (8 * 1024 * 1024)    // 8MB 测试磁盘
#define PASS_COLOR          "\033[32m"            // 绿色
#define FAIL_COLOR          "\033[31m"            // 红色
#define RESET_COLOR         "\033[0m"             // 重置颜色

// 测试统计
static int total_tests = 0;
static int passed_tests = 0;

/*==============================================================================
 * 测试工具函数
 *============================================================================*/

/**
 * 测试断言宏
 */
#define TEST_ASSERT(condition, description) \
    do { \
        total_tests++; \
        if (condition) { \
            printf("  [" PASS_COLOR "PASS" RESET_COLOR "] %s\n", description); \
            passed_tests++; \
        } else { \
            printf("  [" FAIL_COLOR "FAIL" RESET_COLOR "] %s\n", description); \
        } \
    } while(0)

/**
 * 开始测试组
 */
void test_group_start(const char* group_name) {
    printf("\n=== %s ===\n", group_name);
}

/**
 * 测试结果总结
 */
void test_summary(void) {
    printf("\n==================== 测试结果总结 ====================\n");
    printf("总测试数: %d\n", total_tests);
    printf("通过数: %d\n", passed_tests);
    printf("失败数: %d\n", total_tests - passed_tests);
    printf("成功率: %.1f%%\n", total_tests > 0 ? (passed_tests * 100.0 / total_tests) : 0.0);
    
    if (passed_tests == total_tests) {
        printf(PASS_COLOR "所有测试通过！" RESET_COLOR "\n");
    } else {
        printf(FAIL_COLOR "部分测试失败！" RESET_COLOR "\n");
    }
    printf("====================================================\n");
}

/*==============================================================================
 * 具体测试函数
 *============================================================================*/

/**
 * 测试磁盘初始化
 */
void test_disk_initialization(void) {
    test_group_start("磁盘初始化测试");
    
    // 删除可能存在的旧文件
    unlink(TEST_DISK_FILE);
    
    // 测试磁盘初始化
    int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_SUCCESS, "磁盘初始化");
    
    // 测试磁盘是否正确初始化
    TEST_ASSERT(disk_is_initialized(), "磁盘初始化状态检查");
    
    // 获取磁盘信息
    uint32_t total_blocks, block_size;
    uint64_t disk_size;
    result = disk_get_info(&total_blocks, &block_size, &disk_size);
    TEST_ASSERT(result == DISK_SUCCESS, "获取磁盘信息");
    TEST_ASSERT(block_size == DISK_BLOCK_SIZE, "块大小验证");
    TEST_ASSERT(disk_size == TEST_DISK_SIZE, "磁盘大小验证");
    TEST_ASSERT(total_blocks == TEST_DISK_SIZE / DISK_BLOCK_SIZE, "总块数验证");
    
    printf("磁盘信息: %u 块，每块 %u 字节，总大小 %lu 字节\n", 
           total_blocks, block_size, disk_size);
}

/**
 * 测试文件系统格式化
 */
void test_filesystem_format(void) {
    test_group_start("文件系统格式化测试");
    
    // 执行格式化
    format_disk();
    
    // 格式化函数是void，我们通过读取验证结果
    printf("文件系统格式化完成\n");
}

/**
 * 测试超级块验证
 */
void test_superblock_verification(void) {
    test_group_start("超级块验证测试");
    
    fs_superblock_t sb;
    fs_error_t result = fs_ops_read_superblock(&sb);
    TEST_ASSERT(result == FS_SUCCESS, "读取超级块");
    
    if (result == FS_SUCCESS) {
        TEST_ASSERT(sb.magic_number == FS_MAGIC_NUMBER, "魔数验证");
        TEST_ASSERT(sb.version == 1, "版本号验证");
        TEST_ASSERT(sb.block_size == BLOCK_SIZE, "块大小验证");
        TEST_ASSERT(sb.total_inodes == FS_DEFAULT_MAX_INODES, "总inode数验证");
        TEST_ASSERT(sb.root_inode == ROOT_INODE_NUM, "根inode号验证");
        TEST_ASSERT(sb.free_inodes < sb.total_inodes, "空闲inode数验证");
        TEST_ASSERT(sb.free_blocks > 0, "空闲块数验证");
        
        printf("超级块详细信息:\n");
        printf("  魔数: 0x%x\n", sb.magic_number);
        printf("  版本: %u\n", sb.version);
        printf("  总块数: %u\n", sb.total_blocks);
        printf("  总inode数: %u (空闲: %u)\n", sb.total_inodes, sb.free_inodes);
        printf("  inode表起始: %u\n", sb.inode_table_start);
        printf("  数据块起始: %u\n", sb.data_blocks_start);
        printf("  根inode: %u\n", sb.root_inode);
    }
}

/**
 * 测试位图验证
 */
void test_bitmap_verification(void) {
    test_group_start("位图验证测试");
    
    // 测试inode位图
    fs_bitmap_t inode_bitmap;
    fs_error_t result = fs_ops_init_bitmap(&inode_bitmap, FS_DEFAULT_MAX_INODES);
    TEST_ASSERT(result == FS_SUCCESS, "inode位图初始化");
    
    if (result == FS_SUCCESS) {
        result = fs_ops_read_bitmap(&inode_bitmap, FS_INODE_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
        TEST_ASSERT(result == FS_SUCCESS, "读取inode位图");
        
        if (result == FS_SUCCESS) {
            TEST_ASSERT(inode_bitmap.total_bits == FS_DEFAULT_MAX_INODES, "inode位图位数验证");
            TEST_ASSERT(inode_bitmap.free_count < FS_DEFAULT_MAX_INODES, "inode位图空闲数验证");
            
            printf("inode位图: %u 位，%u 空闲\n", 
                   inode_bitmap.total_bits, inode_bitmap.free_count);
            
            // 验证根目录inode被标记为已使用
            uint32_t byte_index = ROOT_INODE_NUM / 8;
            uint32_t bit_offset = ROOT_INODE_NUM % 8;
            int root_used = (inode_bitmap.bitmap[byte_index] & (1 << bit_offset)) != 0;
            TEST_ASSERT(root_used, "根目录inode标记为已使用");
        }
        
        free(inode_bitmap.bitmap);
    }
    
    // 测试数据块位图
    uint32_t total_blocks = TEST_DISK_SIZE / DISK_BLOCK_SIZE;
    uint32_t data_blocks_start = FS_INODE_TABLE_START + 
                                (FS_DEFAULT_MAX_INODES * sizeof(fs_inode_t) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32_t data_blocks_count = total_blocks - data_blocks_start;
    
    fs_bitmap_t block_bitmap;
    result = fs_ops_init_bitmap(&block_bitmap, data_blocks_count);
    TEST_ASSERT(result == FS_SUCCESS, "数据块位图初始化");
    
    if (result == FS_SUCCESS) {
        result = fs_ops_read_bitmap(&block_bitmap, FS_DATA_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
        TEST_ASSERT(result == FS_SUCCESS, "读取数据块位图");
        
        if (result == FS_SUCCESS) {
            TEST_ASSERT(block_bitmap.total_bits == data_blocks_count, "数据块位图位数验证");
            TEST_ASSERT(block_bitmap.free_count < data_blocks_count, "数据块位图空闲数验证");
            
            printf("数据块位图: %u 位，%u 空闲\n", 
                   block_bitmap.total_bits, block_bitmap.free_count);
            
            // 验证根目录数据块被标记为已使用
            int first_block_used = (block_bitmap.bitmap[0] & 1) != 0;
            TEST_ASSERT(first_block_used, "根目录数据块标记为已使用");
        }
        
        free(block_bitmap.bitmap);
    }
}

/**
 * 测试根目录验证
 */
void test_root_directory_verification(void) {
    test_group_start("根目录验证测试");
    
    // 读取根目录inode
    fs_superblock_t sb;
    fs_error_t result = fs_ops_read_superblock(&sb);
    TEST_ASSERT(result == FS_SUCCESS, "读取超级块获取根inode信息");
    
    if (result != FS_SUCCESS) {
        return;
    }
    
    // 计算根目录inode在inode表中的位置
    uint32_t inodes_per_block = DISK_BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = sb.inode_table_start + (ROOT_INODE_NUM / inodes_per_block);
    uint32_t inode_offset = (ROOT_INODE_NUM % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取包含根目录inode的块
    char inode_block[DISK_BLOCK_SIZE];
    int disk_result = disk_read_block(inode_block_num, inode_block);
    TEST_ASSERT(disk_result == DISK_SUCCESS, "读取根目录inode块");
    
    if (disk_result != DISK_SUCCESS) {
        return;
    }
    
    // 获取根目录inode
    fs_inode_t *root_inode = (fs_inode_t *)(inode_block + inode_offset);
    
    // 验证根目录inode属性
    TEST_ASSERT(root_inode->inode_number == ROOT_INODE_NUM, "根目录inode号验证");
    TEST_ASSERT(root_inode->file_type == FS_FILE_TYPE_DIRECTORY, "根目录类型验证");
    TEST_ASSERT(root_inode->permissions == FS_ROOT_PERMISSIONS, "根目录权限验证");
    TEST_ASSERT(root_inode->owner_uid == FS_ROOT_UID, "根目录所有者UID验证");
    TEST_ASSERT(root_inode->owner_gid == FS_ROOT_GID, "根目录所有者GID验证");
    TEST_ASSERT(root_inode->link_count == 2, "根目录链接数验证");
    TEST_ASSERT(root_inode->block_count == 1, "根目录块数验证");
    TEST_ASSERT(root_inode->direct_blocks[0] == sb.data_blocks_start, "根目录数据块号验证");
    
    printf("根目录inode信息:\n");
    printf("  inode号: %u\n", root_inode->inode_number);
    printf("  类型: %u (目录)\n", root_inode->file_type);
    printf("  权限: 0%o\n", root_inode->permissions);
    printf("  大小: %lu 字节\n", root_inode->file_size);
    printf("  数据块: %u\n", root_inode->direct_blocks[0]);
    
    // 读取根目录数据块
    char dir_block[DISK_BLOCK_SIZE];
    disk_result = disk_read_block(root_inode->direct_blocks[0], dir_block);
    TEST_ASSERT(disk_result == DISK_SUCCESS, "读取根目录数据块");
    
    if (disk_result == DISK_SUCCESS) {
        fs_dir_entry_t *entries = (fs_dir_entry_t *)dir_block;
        
        // 验证 "." 目录项
        TEST_ASSERT(entries[0].is_valid == 1, "\".\" 目录项有效性");
        TEST_ASSERT(entries[0].inode_number == ROOT_INODE_NUM, "\".\" 目录项inode号");
        TEST_ASSERT(entries[0].file_type == FS_FILE_TYPE_DIRECTORY, "\".\" 目录项类型");
        TEST_ASSERT(strcmp(entries[0].filename, ".") == 0, "\".\" 目录项文件名");
        
        // 验证 ".." 目录项
        TEST_ASSERT(entries[1].is_valid == 1, "\"..\" 目录项有效性");
        TEST_ASSERT(entries[1].inode_number == ROOT_INODE_NUM, "\"..\" 目录项inode号");
        TEST_ASSERT(entries[1].file_type == FS_FILE_TYPE_DIRECTORY, "\"..\" 目录项类型");
        TEST_ASSERT(strcmp(entries[1].filename, "..") == 0, "\"..\" 目录项文件名");
        
        printf("根目录内容:\n");
        printf("  [0] \"%s\" -> inode %u\n", entries[0].filename, entries[0].inode_number);
        printf("  [1] \"%s\" -> inode %u\n", entries[1].filename, entries[1].inode_number);
    }
}

/**
 * 测试数据持久性
 */
void test_data_persistence(void) {
    test_group_start("数据持久性测试");
    
    // 关闭磁盘
    int result = disk_close();
    TEST_ASSERT(result == DISK_SUCCESS, "磁盘关闭");
    
    // 重新打开磁盘
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_SUCCESS, "磁盘重新打开");
    
    // 验证超级块是否仍然有效
    fs_superblock_t sb;
    fs_error_t fs_result = fs_ops_read_superblock(&sb);
    TEST_ASSERT(fs_result == FS_SUCCESS, "重新读取超级块");
    
    if (fs_result == FS_SUCCESS) {
        TEST_ASSERT(sb.magic_number == FS_MAGIC_NUMBER, "持久化后魔数验证");
        TEST_ASSERT(sb.version == 1, "持久化后版本验证");
        TEST_ASSERT(sb.root_inode == ROOT_INODE_NUM, "持久化后根inode验证");
        
        printf("数据持久性验证通过\n");
    }
}

/**
 * 清理测试环境
 */
void test_cleanup(void) {
    test_group_start("清理测试环境");
    
    // 关闭磁盘
    disk_close();
    
    // 删除测试文件
    int result = unlink(TEST_DISK_FILE);
    TEST_ASSERT(result == 0, "删除测试磁盘文件");
    
    printf("测试环境清理完成\n");
}

/*==============================================================================
 * 主测试函数
 *============================================================================*/

int main(void) {
    printf("================== 文件系统格式化测试程序 ==================\n");
    printf("测试文件: %s\n", TEST_DISK_FILE);
    printf("磁盘大小: %d 字节 (%.1f MB)\n", TEST_DISK_SIZE, TEST_DISK_SIZE / (1024.0 * 1024.0));
    printf("===========================================================\n");
    
    // 执行所有测试
    test_disk_initialization();
    test_filesystem_format();
    test_superblock_verification();
    test_bitmap_verification();
    test_root_directory_verification();
    test_data_persistence();
    test_cleanup();
    
    // 打印测试总结
    test_summary();
    
    return (passed_tests == total_tests) ? 0 : 1;
} 