/**
 * 磁盘模拟器测试程序
 * disk_test.c
 * 
 * 测试磁盘模拟器的核心功能，包括初始化、读写操作、错误处理等
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "disk_simulator.h"

#define TEST_DISK_FILE "test_disk.img"
#define TEST_DISK_SIZE (1024 * 1024)  // 1MB磁盘
#define TEST_BLOCK_COUNT (TEST_DISK_SIZE / DISK_BLOCK_SIZE)

/**
 * 测试结果统计
 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} test_results_t;

static test_results_t g_test_results = {0};

/**
 * 测试辅助宏
 */
#define TEST_START(name) \
    do { \
        printf("测试: %s ... ", name); \
        fflush(stdout); \
        g_test_results.total_tests++; \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("通过\n"); \
        g_test_results.passed_tests++; \
    } while(0)

#define TEST_FAIL(reason) \
    do { \
        printf("失败: %s\n", reason); \
        g_test_results.failed_tests++; \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(message); \
            return 0; \
        } \
    } while(0)

/**
 * 清理测试环境
 */
void cleanup_test_env(void) {
    if (disk_is_initialized()) {
        disk_close();
    }
    unlink(TEST_DISK_FILE);
}

/**
 * 测试磁盘初始化
 */
int test_disk_init(void) {
    TEST_START("磁盘初始化");
    
    // 清理环境
    cleanup_test_env();
    
    // 测试无效参数
    int result = disk_init(NULL, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_ERROR_INVALID_PARAM, "空文件名应该返回无效参数错误");
    
    result = disk_init(TEST_DISK_FILE, 0);
    TEST_ASSERT(result == DISK_ERROR_INVALID_PARAM, "零大小应该返回无效参数错误");
    
    result = disk_init(TEST_DISK_FILE, 1023);  // 非块对齐
    TEST_ASSERT(result == DISK_ERROR_INVALID_PARAM, "非块对齐大小应该返回无效参数错误");
    
    // 测试正常初始化
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_SUCCESS, "正常初始化应该成功");
    TEST_ASSERT(disk_is_initialized(), "磁盘应该已初始化");
    
    // 测试重复初始化
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_ERROR_ALREADY_INIT, "重复初始化应该返回已初始化错误");
    
    // 验证磁盘信息
    uint32_t total_blocks, block_size;
    uint64_t disk_size;
    result = disk_get_info(&total_blocks, &block_size, &disk_size);
    TEST_ASSERT(result == DISK_SUCCESS, "获取磁盘信息应该成功");
    TEST_ASSERT(total_blocks == TEST_BLOCK_COUNT, "块数应该正确");
    TEST_ASSERT(block_size == DISK_BLOCK_SIZE, "块大小应该正确");
    TEST_ASSERT(disk_size == TEST_DISK_SIZE, "磁盘大小应该正确");
    
    TEST_PASS();
    return 1;
}

/**
 * 测试磁盘读写操作
 */
int test_disk_read_write(void) {
    TEST_START("磁盘读写操作");
    
    // 确保磁盘已初始化
    if (!disk_is_initialized()) {
        int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
        TEST_ASSERT(result == DISK_SUCCESS, "磁盘初始化失败");
    }
    
    // 准备测试数据
    char write_buffer[DISK_BLOCK_SIZE];
    char read_buffer[DISK_BLOCK_SIZE];
    
    // 填充测试数据
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        write_buffer[i] = (char)(i % 256);
    }
    
    // 测试写入操作
    int result = disk_write_block(0, write_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "写入块0应该成功");
    
    result = disk_write_block(TEST_BLOCK_COUNT - 1, write_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "写入最后一个块应该成功");
    
    // 测试越界写入
    result = disk_write_block(TEST_BLOCK_COUNT, write_buffer);
    TEST_ASSERT(result == DISK_ERROR_BLOCK_RANGE, "越界写入应该返回块范围错误");
    
    result = disk_write_block(-1, write_buffer);
    TEST_ASSERT(result == DISK_ERROR_BLOCK_RANGE, "负数块号应该返回块范围错误");
    
    // 测试无效参数
    result = disk_write_block(0, NULL);
    TEST_ASSERT(result == DISK_ERROR_INVALID_PARAM, "空数据指针应该返回无效参数错误");
    
    // 测试读取操作
    result = disk_read_block(0, read_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "读取块0应该成功");
    
    // 验证读取的数据
    TEST_ASSERT(memcmp(write_buffer, read_buffer, DISK_BLOCK_SIZE) == 0, 
                "读取的数据应该与写入的数据一致");
    
    // 测试越界读取
    result = disk_read_block(TEST_BLOCK_COUNT, read_buffer);
    TEST_ASSERT(result == DISK_ERROR_BLOCK_RANGE, "越界读取应该返回块范围错误");
    
    // 测试无效参数
    result = disk_read_block(0, NULL);
    TEST_ASSERT(result == DISK_ERROR_INVALID_PARAM, "空缓冲区指针应该返回无效参数错误");
    
    TEST_PASS();
    return 1;
}

/**
 * 测试磁盘格式化
 */
int test_disk_format(void) {
    TEST_START("磁盘格式化");
    
    // 确保磁盘已初始化
    if (!disk_is_initialized()) {
        int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
        TEST_ASSERT(result == DISK_SUCCESS, "磁盘初始化失败");
    }
    
    // 使用模式0xAA格式化磁盘
    int result = disk_format(0xAA);
    TEST_ASSERT(result == DISK_SUCCESS, "磁盘格式化应该成功");
    
    // 验证格式化结果
    char read_buffer[DISK_BLOCK_SIZE];
    result = disk_read_block(0, read_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "读取格式化后的块应该成功");
    
    // 检查所有字节是否为0xAA
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        TEST_ASSERT((unsigned char)read_buffer[i] == 0xAA, 
                    "格式化后的数据应该全部为0xAA");
    }
    
    // 测试随机块
    result = disk_read_block(TEST_BLOCK_COUNT / 2, read_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "读取中间块应该成功");
    
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        TEST_ASSERT((unsigned char)read_buffer[i] == 0xAA, 
                    "中间块的数据也应该全部为0xAA");
    }
    
    TEST_PASS();
    return 1;
}

/**
 * 测试多块操作
 */
int test_multi_block_operations(void) {
    TEST_START("多块操作");
    
    // 确保磁盘已初始化
    if (!disk_is_initialized()) {
        int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
        TEST_ASSERT(result == DISK_SUCCESS, "磁盘初始化失败");
    }
    
    const int block_count = 5;
    char write_data[block_count * DISK_BLOCK_SIZE];
    char read_data[block_count * DISK_BLOCK_SIZE];
    
    // 填充测试数据
    for (int i = 0; i < block_count * DISK_BLOCK_SIZE; i++) {
        write_data[i] = (char)(i % 256);
    }
    
    // 测试多块写入
    int result = disk_write_blocks(10, block_count, write_data);
    TEST_ASSERT(result == DISK_SUCCESS, "多块写入应该成功");
    
    // 测试多块读取
    result = disk_read_blocks(10, block_count, read_data);
    TEST_ASSERT(result == DISK_SUCCESS, "多块读取应该成功");
    
    // 验证数据
    TEST_ASSERT(memcmp(write_data, read_data, block_count * DISK_BLOCK_SIZE) == 0,
                "多块读取的数据应该与写入的数据一致");
    
    // 测试越界多块操作
    result = disk_write_blocks(TEST_BLOCK_COUNT - 1, 2, write_data);
    TEST_ASSERT(result == DISK_ERROR_BLOCK_RANGE, "越界多块写入应该失败");
    
    TEST_PASS();
    return 1;
}

/**
 * 测试辅助函数
 */
int test_utility_functions(void) {
    TEST_START("辅助函数");
    
    // 确保磁盘已初始化
    if (!disk_is_initialized()) {
        int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
        TEST_ASSERT(result == DISK_SUCCESS, "磁盘初始化失败");
    }
    
    // 测试块验证函数
    TEST_ASSERT(disk_is_valid_block(0), "块0应该有效");
    TEST_ASSERT(disk_is_valid_block(TEST_BLOCK_COUNT - 1), "最后一个块应该有效");
    TEST_ASSERT(!disk_is_valid_block(TEST_BLOCK_COUNT), "越界块应该无效");
    TEST_ASSERT(!disk_is_valid_block(-1), "负数块号应该无效");
    
    // 测试获取块数
    uint32_t block_count = disk_get_block_count();
    TEST_ASSERT(block_count == TEST_BLOCK_COUNT, "获取的块数应该正确");
    
    // 测试清零块
    int result = disk_zero_block(20);
    TEST_ASSERT(result == DISK_SUCCESS, "清零块应该成功");
    
    char read_buffer[DISK_BLOCK_SIZE];
    result = disk_read_block(20, read_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "读取清零块应该成功");
    
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        TEST_ASSERT(read_buffer[i] == 0, "清零块的所有字节应该为0");
    }
    
    // 测试复制块
    char test_data[DISK_BLOCK_SIZE];
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        test_data[i] = (char)(i % 256);
    }
    
    result = disk_write_block(30, test_data);
    TEST_ASSERT(result == DISK_SUCCESS, "写入源块应该成功");
    
    result = disk_copy_block(30, 31);
    TEST_ASSERT(result == DISK_SUCCESS, "复制块应该成功");
    
    result = disk_read_block(31, read_buffer);
    TEST_ASSERT(result == DISK_SUCCESS, "读取目标块应该成功");
    
    TEST_ASSERT(memcmp(test_data, read_buffer, DISK_BLOCK_SIZE) == 0,
                "复制的数据应该一致");
    
    // 测试错误码转换
    const char* error_str = disk_error_to_string(DISK_SUCCESS);
    TEST_ASSERT(error_str != NULL, "错误码转换应该返回非空字符串");
    
    TEST_PASS();
    return 1;
}

/**
 * 测试统计功能
 */
int test_statistics(void) {
    TEST_START("统计功能");
    
    // 确保磁盘已初始化
    if (!disk_is_initialized()) {
        int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
        TEST_ASSERT(result == DISK_SUCCESS, "磁盘初始化失败");
    }
    
    // 重置统计
    int result = disk_reset_stats();
    TEST_ASSERT(result == DISK_SUCCESS, "重置统计应该成功");
    
    // 获取初始统计
    disk_stats_t stats;
    result = disk_get_stats(&stats);
    TEST_ASSERT(result == DISK_SUCCESS, "获取统计应该成功");
    TEST_ASSERT(stats.total_reads == 0, "初始读取次数应该为0");
    TEST_ASSERT(stats.total_writes == 0, "初始写入次数应该为0");
    
    // 执行一些操作
    char buffer[DISK_BLOCK_SIZE];
    memset(buffer, 0x55, DISK_BLOCK_SIZE);
    
    for (int i = 0; i < 5; i++) {
        disk_write_block(i, buffer);
        disk_read_block(i, buffer);
    }
    
    // 检查统计更新
    result = disk_get_stats(&stats);
    TEST_ASSERT(result == DISK_SUCCESS, "获取更新后统计应该成功");
    TEST_ASSERT(stats.total_reads == 5, "读取次数应该为5");
    TEST_ASSERT(stats.total_writes == 5, "写入次数应该为5");
    TEST_ASSERT(stats.bytes_read == 5 * DISK_BLOCK_SIZE, "读取字节数应该正确");
    TEST_ASSERT(stats.bytes_written == 5 * DISK_BLOCK_SIZE, "写入字节数应该正确");
    
    TEST_PASS();
    return 1;
}

/**
 * 测试磁盘持久性
 */
int test_disk_persistence(void) {
    TEST_START("磁盘持久性");
    
    // 创建新磁盘
    cleanup_test_env();
    int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_SUCCESS, "初始化新磁盘应该成功");
    
    // 写入测试数据
    char test_data[DISK_BLOCK_SIZE];
    for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
        test_data[i] = (char)(i % 256);
    }
    
    result = disk_write_block(50, test_data);
    TEST_ASSERT(result == DISK_SUCCESS, "写入测试数据应该成功");
    
    // 同步并关闭磁盘
    result = disk_sync();
    TEST_ASSERT(result == DISK_SUCCESS, "同步磁盘应该成功");
    
    result = disk_close();
    TEST_ASSERT(result == DISK_SUCCESS, "关闭磁盘应该成功");
    
    // 重新打开磁盘
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    TEST_ASSERT(result == DISK_SUCCESS, "重新打开磁盘应该成功");
    
    // 验证数据仍然存在
    char read_data[DISK_BLOCK_SIZE];
    result = disk_read_block(50, read_data);
    TEST_ASSERT(result == DISK_SUCCESS, "读取持久化数据应该成功");
    
    TEST_ASSERT(memcmp(test_data, read_data, DISK_BLOCK_SIZE) == 0,
                "持久化的数据应该保持不变");
    
    TEST_PASS();
    return 1;
}

/**
 * 打印测试结果
 */
void print_test_results(void) {
    printf("\n=== 测试结果总结 ===\n");
    printf("总测试数: %d\n", g_test_results.total_tests);
    printf("通过测试: %d\n", g_test_results.passed_tests);
    printf("失败测试: %d\n", g_test_results.failed_tests);
    printf("成功率: %.1f%%\n", 
           g_test_results.total_tests > 0 ? 
           (100.0 * g_test_results.passed_tests / g_test_results.total_tests) : 0.0);
    
    if (g_test_results.failed_tests == 0) {
        printf("🎉 所有测试都通过了！\n");
    } else {
        printf("⚠️ 有 %d 个测试失败\n", g_test_results.failed_tests);
    }
    printf("===================\n\n");
}

/**
 * 主测试函数
 */
int main(void) {
    printf("磁盘模拟器测试程序\n");
    printf("==================\n\n");
    
    // 运行所有测试
    test_disk_init();
    test_disk_read_write();
    test_disk_format();
    test_multi_block_operations();
    test_utility_functions();
    test_statistics();
    test_disk_persistence();
    
    // 清理环境
    cleanup_test_env();
    
    // 打印测试结果
    print_test_results();
    
    // 返回适当的退出码
    return (g_test_results.failed_tests == 0) ? 0 : 1;
} 