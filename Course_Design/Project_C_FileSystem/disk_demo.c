/**
 * 磁盘模拟器演示程序
 * disk_demo.c
 * 
 * 演示磁盘模拟器的基本功能和使用方法
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk_simulator.h"

#define DEMO_DISK_FILE "demo_disk.img"
#define DEMO_DISK_SIZE (2 * 1024 * 1024)  // 2MB磁盘

/**
 * 演示基本的磁盘操作
 */
void demo_basic_operations(void) {
    printf("\n=== 基本磁盘操作演示 ===\n");
    
    // 初始化磁盘
    printf("1. 初始化磁盘 (%s, %d 字节)...\n", DEMO_DISK_FILE, DEMO_DISK_SIZE);
    int result = disk_init(DEMO_DISK_FILE, DEMO_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("错误: 磁盘初始化失败 - %s\n", disk_error_to_string(result));
        return;
    }
    printf("   ✓ 磁盘初始化成功\n");
    
    // 获取磁盘信息
    uint32_t total_blocks, block_size;
    uint64_t disk_size;
    disk_get_info(&total_blocks, &block_size, &disk_size);
    printf("2. 磁盘信息:\n");
    printf("   - 总块数: %u\n", total_blocks);
    printf("   - 块大小: %u 字节\n", block_size);
    printf("   - 磁盘大小: %lu 字节 (%.2f MB)\n", 
           disk_size, disk_size / (1024.0 * 1024.0));
    
    // 写入测试数据
    printf("3. 写入测试数据...\n");
    char write_buffer[DISK_BLOCK_SIZE];
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        write_buffer[i] = (char)(i % 256);
    }
    
    result = disk_write_block(0, write_buffer);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 成功写入块0\n");
    } else {
        printf("   ✗ 写入失败 - %s\n", disk_error_to_string(result));
    }
    
    // 读取数据
    printf("4. 读取数据...\n");
    char read_buffer[DISK_BLOCK_SIZE];
    result = disk_read_block(0, read_buffer);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 成功读取块0\n");
        
        // 验证数据
        if (memcmp(write_buffer, read_buffer, DISK_BLOCK_SIZE) == 0) {
            printf("   ✓ 数据验证通过\n");
        } else {
            printf("   ✗ 数据验证失败\n");
        }
    } else {
        printf("   ✗ 读取失败 - %s\n", disk_error_to_string(result));
    }
}

/**
 * 演示多块操作
 */
void demo_multi_block_operations(void) {
    printf("\n=== 多块操作演示 ===\n");
    
    const int block_count = 10;
    char multi_write_data[block_count * DISK_BLOCK_SIZE];
    char multi_read_data[block_count * DISK_BLOCK_SIZE];
    
    // 填充测试数据
    printf("1. 准备 %d 块测试数据...\n", block_count);
    for (int i = 0; i < block_count * DISK_BLOCK_SIZE; i++) {
        multi_write_data[i] = (char)((i / DISK_BLOCK_SIZE + 'A') % 26 + 'A');
    }
    
    // 写入多个块
    printf("2. 写入多个连续块 (块10-19)...\n");
    int result = disk_write_blocks(10, block_count, multi_write_data);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 成功写入 %d 个块\n", block_count);
    } else {
        printf("   ✗ 多块写入失败 - %s\n", disk_error_to_string(result));
        return;
    }
    
    // 读取多个块
    printf("3. 读取多个连续块...\n");
    result = disk_read_blocks(10, block_count, multi_read_data);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 成功读取 %d 个块\n", block_count);
        
        // 验证数据
        if (memcmp(multi_write_data, multi_read_data, block_count * DISK_BLOCK_SIZE) == 0) {
            printf("   ✓ 多块数据验证通过\n");
        } else {
            printf("   ✗ 多块数据验证失败\n");
        }
    } else {
        printf("   ✗ 多块读取失败 - %s\n", disk_error_to_string(result));
    }
}

/**
 * 演示工具函数
 */
void demo_utility_functions(void) {
    printf("\n=== 工具函数演示 ===\n");
    
    // 清零块
    printf("1. 清零块50...\n");
    int result = disk_zero_block(50);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 块50已清零\n");
        
        // 验证清零结果
        char verify_buffer[DISK_BLOCK_SIZE];
        disk_read_block(50, verify_buffer);
        
        int all_zero = 1;
        for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
            if (verify_buffer[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        
        if (all_zero) {
            printf("   ✓ 清零验证通过\n");
        } else {
            printf("   ✗ 清零验证失败\n");
        }
    } else {
        printf("   ✗ 清零失败 - %s\n", disk_error_to_string(result));
    }
    
    // 复制块
    printf("2. 复制块0到块51...\n");
    result = disk_copy_block(0, 51);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 块复制成功\n");
        
        // 验证复制结果
        char source_buffer[DISK_BLOCK_SIZE];
        char dest_buffer[DISK_BLOCK_SIZE];
        
        disk_read_block(0, source_buffer);
        disk_read_block(51, dest_buffer);
        
        if (memcmp(source_buffer, dest_buffer, DISK_BLOCK_SIZE) == 0) {
            printf("   ✓ 复制验证通过\n");
        } else {
            printf("   ✗ 复制验证失败\n");
        }
    } else {
        printf("   ✗ 复制失败 - %s\n", disk_error_to_string(result));
    }
    
    // 块验证
    printf("3. 块号验证...\n");
    uint32_t block_count = disk_get_block_count();
    printf("   - 有效块范围: 0 - %u\n", block_count - 1);
    printf("   - 块0有效性: %s\n", disk_is_valid_block(0) ? "有效" : "无效");
    printf("   - 块%u有效性: %s\n", block_count - 1, 
           disk_is_valid_block(block_count - 1) ? "有效" : "无效");
    printf("   - 块%u有效性: %s\n", block_count, 
           disk_is_valid_block(block_count) ? "有效" : "无效");
}

/**
 * 演示统计功能
 */
void demo_statistics(void) {
    printf("\n=== 统计功能演示 ===\n");
    
    // 重置统计
    printf("1. 重置统计信息...\n");
    disk_reset_stats();
    printf("   ✓ 统计信息已重置\n");
    
    // 执行一些操作来生成统计数据
    printf("2. 执行操作生成统计数据...\n");
    char buffer[DISK_BLOCK_SIZE];
    memset(buffer, 0xAA, DISK_BLOCK_SIZE);
    
    for (int i = 100; i < 110; i++) {
        disk_write_block(i, buffer);
        disk_read_block(i, buffer);
    }
    printf("   ✓ 完成10次读写操作\n");
    
    // 显示统计信息
    printf("3. 统计信息:\n");
    disk_stats_t stats;
    disk_get_stats(&stats);
    
    printf("   - 总读取次数: %lu\n", stats.total_reads);
    printf("   - 总写入次数: %lu\n", stats.total_writes);
    printf("   - 读取字节数: %lu\n", stats.bytes_read);
    printf("   - 写入字节数: %lu\n", stats.bytes_written);
    printf("   - 读取错误: %lu\n", stats.read_errors);
    printf("   - 写入错误: %lu\n", stats.write_errors);
    printf("   - 平均读取时间: %.6f 秒\n", stats.avg_read_time);
    printf("   - 平均写入时间: %.6f 秒\n", stats.avg_write_time);
}

/**
 * 演示磁盘格式化
 */
void demo_disk_format(void) {
    printf("\n=== 磁盘格式化演示 ===\n");
    
    printf("1. 使用模式0x55格式化磁盘...\n");
    printf("   注意: 这将清除所有数据！\n");
    
    int result = disk_format(0x55);
    if (result == DISK_SUCCESS) {
        printf("   ✓ 磁盘格式化完成\n");
        
        // 验证格式化结果
        char verify_buffer[DISK_BLOCK_SIZE];
        disk_read_block(0, verify_buffer);
        disk_read_block(500, verify_buffer);  // 随机检查一个块
        
        int pattern_correct = 1;
        for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
            if ((unsigned char)verify_buffer[i] != 0x55) {
                pattern_correct = 0;
                break;
            }
        }
        
        if (pattern_correct) {
            printf("   ✓ 格式化验证通过\n");
        } else {
            printf("   ✗ 格式化验证失败\n");
        }
    } else {
        printf("   ✗ 格式化失败 - %s\n", disk_error_to_string(result));
    }
}

/**
 * 主演示函数
 */
int main(void) {
    printf("磁盘模拟器功能演示\n");
    printf("===================\n");
    
    // 确保没有遗留文件
    unlink(DEMO_DISK_FILE);
    
    // 运行各个演示
    demo_basic_operations();
    demo_multi_block_operations();
    demo_utility_functions();
    demo_statistics();
    demo_disk_format();
    
    // 显示最终状态
    printf("\n=== 磁盘最终状态 ===\n");
    disk_print_status();
    
    // 清理
    printf("=== 清理 ===\n");
    printf("1. 同步磁盘...\n");
    int result = disk_sync();
    if (result == DISK_SUCCESS) {
        printf("   ✓ 磁盘同步成功\n");
    } else {
        printf("   ✗ 磁盘同步失败 - %s\n", disk_error_to_string(result));
    }
    
    printf("2. 关闭磁盘...\n");
    result = disk_close();
    if (result == DISK_SUCCESS) {
        printf("   ✓ 磁盘关闭成功\n");
    } else {
        printf("   ✗ 磁盘关闭失败 - %s\n", disk_error_to_string(result));
    }
    
    printf("3. 删除演示文件...\n");
    if (unlink(DEMO_DISK_FILE) == 0) {
        printf("   ✓ 演示文件删除成功\n");
    } else {
        printf("   ✗ 演示文件删除失败\n");
    }
    
    printf("\n演示完成！\n");
    return 0;
} 