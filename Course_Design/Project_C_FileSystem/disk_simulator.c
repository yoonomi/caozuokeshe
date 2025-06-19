/**
 * Disk Simulator Implementation
 * disk_simulator.c
 * 
 * 磁盘模拟器实现 - 使用单个主机OS文件模拟基于块的磁盘存储
 */

#include "disk_simulator.h"

/* 全局磁盘状态 */
disk_state_t g_disk_state = {0};

/*==============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * 计算简单校验和
 */
static uint32_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum += bytes[i];
        checksum = (checksum << 1) | (checksum >> 31);
    }
    
    return checksum;
}

/**
 * 更新读操作统计
 */
static void update_stats_read(uint64_t bytes, double elapsed_time) {
    g_disk_state.stats.total_reads++;
    g_disk_state.stats.bytes_read += bytes;
    g_disk_state.stats.last_operation_time = time(NULL);
    
    if (g_disk_state.stats.total_reads == 1) {
        g_disk_state.stats.avg_read_time = elapsed_time;
    } else {
        g_disk_state.stats.avg_read_time = 
            (g_disk_state.stats.avg_read_time * 0.9) + (elapsed_time * 0.1);
    }
}

/**
 * 更新写操作统计
 */
static void update_stats_write(uint64_t bytes, double elapsed_time) {
    g_disk_state.stats.total_writes++;
    g_disk_state.stats.bytes_written += bytes;
    g_disk_state.stats.last_operation_time = time(NULL);
    g_disk_state.is_dirty = 1;
    
    if (g_disk_state.stats.total_writes == 1) {
        g_disk_state.stats.avg_write_time = elapsed_time;
    } else {
        g_disk_state.stats.avg_write_time = 
            (g_disk_state.stats.avg_write_time * 0.9) + (elapsed_time * 0.1);
    }
}

/**
 * 获取当前时间（高精度）
 */
static double get_current_time(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ts.tv_sec + ts.tv_nsec / 1000000000.0;
    }
    return (double)time(NULL);
}

/**
 * 创建磁盘头部
 */
static int create_disk_header(disk_header_t* header, uint32_t total_blocks) {
    if (!header) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    memset(header, 0, sizeof(disk_header_t));
    
    header->magic_number = DISK_MAGIC_HEADER;
    header->version = DISK_VERSION;
    header->block_size = DISK_BLOCK_SIZE;
    header->total_blocks = total_blocks;
    header->disk_size = (uint64_t)total_blocks * DISK_BLOCK_SIZE;
    header->created_time = time(NULL);
    header->last_access_time = header->created_time;
    
    // 只对稳定的字段计算校验和（排除时间戳和校验和字段）
    uint32_t stable_size = offsetof(disk_header_t, created_time);
    header->checksum = calculate_checksum(header, stable_size);
    
    return DISK_SUCCESS;
}

/**
 * 验证磁盘头部
 */
static int validate_disk_header(const disk_header_t* header) {
    if (!header) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    if (header->magic_number != DISK_MAGIC_HEADER) {
        return DISK_ERROR_CORRUPTED;
    }
    
    if (header->version != DISK_VERSION) {
        return DISK_ERROR_CORRUPTED;
    }
    
    if (header->block_size != DISK_BLOCK_SIZE) {
        return DISK_ERROR_CORRUPTED;
    }
    
    // 只验证稳定字段的校验和
    uint32_t stable_size = offsetof(disk_header_t, created_time);
    uint32_t calculated_checksum = calculate_checksum(header, stable_size);
    if (calculated_checksum != header->checksum) {
        return DISK_ERROR_CORRUPTED;
    }
    
    return DISK_SUCCESS;
}

/*==============================================================================
 * 核心磁盘操作实现
 *============================================================================*/

/**
 * 初始化磁盘模拟器
 */
int disk_init(const char* filename, int disk_size) {
    // 参数验证
    if (!filename || disk_size <= 0) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    if (g_disk_state.is_initialized) {
        return DISK_ERROR_ALREADY_INIT;
    }
    
    // 检查磁盘大小是否块对齐
    if (!DISK_IS_BLOCK_ALIGNED(disk_size)) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    // 计算块数
    uint32_t total_blocks = disk_size / DISK_BLOCK_SIZE;
    if (total_blocks == 0) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    // 初始化磁盘状态
    memset(&g_disk_state, 0, sizeof(g_disk_state));
    strncpy(g_disk_state.filename, filename, DISK_MAX_FILENAME_LEN - 1);
    g_disk_state.filename[DISK_MAX_FILENAME_LEN - 1] = '\0';
    
    // 检查文件是否存在
    struct stat file_stat;
    int file_exists = (stat(filename, &file_stat) == 0);
    
    if (file_exists) {
        // 打开现有文件
        g_disk_state.fd = open(filename, O_RDWR);
        if (g_disk_state.fd == -1) {
            return DISK_ERROR_FILE_OPEN;
        }
        
        // 读取并验证头部
        disk_header_t header;
        ssize_t bytes_read = read(g_disk_state.fd, &header, sizeof(header));
        if (bytes_read != sizeof(header)) {
            close(g_disk_state.fd);
            return DISK_ERROR_FILE_READ;
        }
        
        int validation_result = validate_disk_header(&header);
        if (validation_result != DISK_SUCCESS) {
            close(g_disk_state.fd);
            return validation_result;
        }
        

        
        // 从头部更新状态
        g_disk_state.total_blocks = header.total_blocks;
        g_disk_state.disk_size = header.disk_size;
        
        // 验证文件大小
        uint64_t expected_size = DISK_TOTAL_FILE_SIZE(header.total_blocks);
        if ((uint64_t)file_stat.st_size < expected_size) {
            close(g_disk_state.fd);
            return DISK_ERROR_CORRUPTED;
        }
        
    } else {
        // 创建新文件
        g_disk_state.fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0644);
        if (g_disk_state.fd == -1) {
            return DISK_ERROR_FILE_CREATE;
        }
        
        // 创建并写入头部
        disk_header_t header;
        int result = create_disk_header(&header, total_blocks);
        if (result != DISK_SUCCESS) {
            close(g_disk_state.fd);
            unlink(filename);
            return result;
        }
        
        ssize_t bytes_written = write(g_disk_state.fd, &header, sizeof(header));
        if (bytes_written != sizeof(header)) {
            close(g_disk_state.fd);
            unlink(filename);
            return DISK_ERROR_FILE_WRITE;
        }
        
        // 扩展文件到完整大小
        uint64_t file_size = DISK_TOTAL_FILE_SIZE(total_blocks);
        if (lseek(g_disk_state.fd, file_size - 1, SEEK_SET) == -1) {
            close(g_disk_state.fd);
            unlink(filename);
            return DISK_ERROR_FILE_SEEK;
        }
        
        char zero = 0;
        if (write(g_disk_state.fd, &zero, 1) != 1) {
            close(g_disk_state.fd);
            unlink(filename);
            return DISK_ERROR_FILE_WRITE;
        }
        
        g_disk_state.total_blocks = total_blocks;
        g_disk_state.disk_size = disk_size;
    }
    
    // 完成初始化
    g_disk_state.block_size = DISK_BLOCK_SIZE;
    g_disk_state.is_initialized = 1;
    g_disk_state.is_read_only = 0;
    g_disk_state.is_dirty = 0;
    g_disk_state.auto_sync = 0;
    g_disk_state.last_sync_time = time(NULL);
    
    return DISK_SUCCESS;
}

/**
 * 写入一个数据块
 */
int disk_write_block(int block_num, const char* data) {
    // 参数验证
    if (!data) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    if (g_disk_state.is_read_only) {
        return DISK_ERROR_IO;
    }
    
    if (!disk_check_block_bounds(block_num)) {
        return DISK_ERROR_BLOCK_RANGE;
    }
    
    double start_time = get_current_time();
    
    // 计算文件偏移
    uint64_t offset = DISK_BLOCK_TO_OFFSET(block_num);
    
    // 定位到块位置
    if (lseek(g_disk_state.fd, offset, SEEK_SET) == -1) {
        g_disk_state.stats.write_errors++;
        return DISK_ERROR_FILE_SEEK;
    }
    
    // 写入块数据
    ssize_t bytes_written = write(g_disk_state.fd, data, DISK_BLOCK_SIZE);
    if (bytes_written != DISK_BLOCK_SIZE) {
        g_disk_state.stats.write_errors++;
        return DISK_ERROR_FILE_WRITE;
    }
    
    // 更新统计
    double elapsed_time = get_current_time() - start_time;
    update_stats_write(DISK_BLOCK_SIZE, elapsed_time);
    
    // 自动同步（如果启用）
    if (g_disk_state.auto_sync) {
        fsync(g_disk_state.fd);
    }
    
    return DISK_SUCCESS;
}

/**
 * 读取一个数据块
 */
int disk_read_block(int block_num, char* buffer) {
    // 参数验证
    if (!buffer) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    if (!disk_check_block_bounds(block_num)) {
        return DISK_ERROR_BLOCK_RANGE;
    }
    
    double start_time = get_current_time();
    
    // 计算文件偏移
    uint64_t offset = DISK_BLOCK_TO_OFFSET(block_num);
    
    // 定位到块位置
    if (lseek(g_disk_state.fd, offset, SEEK_SET) == -1) {
        g_disk_state.stats.read_errors++;
        return DISK_ERROR_FILE_SEEK;
    }
    
    // 读取块数据
    ssize_t bytes_read = read(g_disk_state.fd, buffer, DISK_BLOCK_SIZE);
    if (bytes_read != DISK_BLOCK_SIZE) {
        g_disk_state.stats.read_errors++;
        return DISK_ERROR_FILE_READ;
    }
    
    // 更新统计
    double elapsed_time = get_current_time() - start_time;
    update_stats_read(DISK_BLOCK_SIZE, elapsed_time);
    
    return DISK_SUCCESS;
}

/*==============================================================================
 * 扩展磁盘操作
 *============================================================================*/

/**
 * 关闭和清理磁盘模拟器
 */
int disk_close(void) {
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    // 同步待写入数据
    if (g_disk_state.is_dirty) {
        disk_sync();
    }
    
    // 关闭文件描述符
    if (g_disk_state.fd != -1) {
        close(g_disk_state.fd);
    }
    
    // 重置状态
    memset(&g_disk_state, 0, sizeof(g_disk_state));
    g_disk_state.fd = -1;
    
    return DISK_SUCCESS;
}

/**
 * 同步磁盘写入
 */
int disk_sync(void) {
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    if (g_disk_state.fd == -1) {
        return DISK_ERROR_IO;
    }
    
    // 强制同步
    if (fsync(g_disk_state.fd) == -1) {
        return DISK_ERROR_IO;
    }
    
    g_disk_state.is_dirty = 0;
    g_disk_state.last_sync_time = time(NULL);
    
    return DISK_SUCCESS;
}

/**
 * 获取磁盘信息
 */
int disk_get_info(uint32_t* total_blocks, uint32_t* block_size, uint64_t* disk_size) {
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    if (total_blocks) {
        *total_blocks = g_disk_state.total_blocks;
    }
    
    if (block_size) {
        *block_size = g_disk_state.block_size;
    }
    
    if (disk_size) {
        *disk_size = g_disk_state.disk_size;
    }
    
    return DISK_SUCCESS;
}

/**
 * 获取磁盘统计
 */
int disk_get_stats(disk_stats_t* stats) {
    if (!stats) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    *stats = g_disk_state.stats;
    return DISK_SUCCESS;
}

/**
 * 重置磁盘统计
 */
int disk_reset_stats(void) {
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    memset(&g_disk_state.stats, 0, sizeof(g_disk_state.stats));
    return DISK_SUCCESS;
}

/*==============================================================================
 * 工具函数
 *============================================================================*/

/**
 * 错误码转字符串
 */
const char* disk_error_to_string(disk_error_t error) {
    switch (error) {
        case DISK_SUCCESS:              return "操作成功";
        case DISK_ERROR_INVALID_PARAM:  return "无效参数";
        case DISK_ERROR_FILE_OPEN:      return "文件打开失败";
        case DISK_ERROR_FILE_CREATE:    return "文件创建失败";
        case DISK_ERROR_FILE_READ:      return "文件读取失败";
        case DISK_ERROR_FILE_WRITE:     return "文件写入失败";
        case DISK_ERROR_FILE_SEEK:      return "文件定位失败";
        case DISK_ERROR_BLOCK_RANGE:    return "块号超出范围";
        case DISK_ERROR_NOT_INIT:       return "磁盘未初始化";
        case DISK_ERROR_ALREADY_INIT:   return "磁盘已初始化";
        case DISK_ERROR_DISK_FULL:      return "磁盘已满";
        case DISK_ERROR_IO:             return "I/O错误";
        case DISK_ERROR_CORRUPTED:      return "磁盘数据损坏";
        default:                        return "未知错误";
    }
}

/**
 * 检查磁盘是否已初始化
 */
int disk_is_initialized(void) {
    return g_disk_state.is_initialized;
}

/**
 * 获取当前磁盘块数
 */
uint32_t disk_get_block_count(void) {
    return g_disk_state.is_initialized ? g_disk_state.total_blocks : 0;
}

/**
 * 验证块号
 */
int disk_is_valid_block(int block_num) {
    return disk_check_block_bounds(block_num);
}

/**
 * 使用模式格式化磁盘
 */
int disk_format(uint8_t pattern) {
    if (!g_disk_state.is_initialized) {
        return DISK_ERROR_NOT_INIT;
    }
    
    if (g_disk_state.is_read_only) {
        return DISK_ERROR_IO;
    }
    
    // 创建模式块
    char block_data[DISK_BLOCK_SIZE];
    memset(block_data, pattern, DISK_BLOCK_SIZE);
    
    // 写入模式到所有块
    for (uint32_t i = 0; i < g_disk_state.total_blocks; i++) {
        int result = disk_write_block(i, block_data);
        if (result != DISK_SUCCESS) {
            return result;
        }
    }
    
    // 强制同步
    return disk_sync();
}

/**
 * 打印磁盘状态
 */
void disk_print_status(void) {
    printf("\n=== 磁盘模拟器状态 ===\n");
    
    if (!g_disk_state.is_initialized) {
        printf("状态: 未初始化\n");
        printf("====================\n\n");
        return;
    }
    
    printf("文件名: %s\n", g_disk_state.filename);
    printf("状态: %s\n", g_disk_state.is_initialized ? "已初始化" : "未初始化");
    printf("模式: %s\n", g_disk_state.is_read_only ? "只读" : "读写");
    printf("块大小: %u 字节\n", g_disk_state.block_size);
    printf("总块数: %u\n", g_disk_state.total_blocks);
    printf("磁盘大小: %lu 字节 (%.2f MB)\n", 
           g_disk_state.disk_size, g_disk_state.disk_size / (1024.0 * 1024.0));
    printf("脏标志: %s\n", g_disk_state.is_dirty ? "是" : "否");
    printf("自动同步: %s\n", g_disk_state.auto_sync ? "启用" : "禁用");
    
    printf("\n--- 统计信息 ---\n");
    printf("总读取次数: %lu\n", g_disk_state.stats.total_reads);
    printf("总写入次数: %lu\n", g_disk_state.stats.total_writes);
    printf("读取字节数: %lu\n", g_disk_state.stats.bytes_read);
    printf("写入字节数: %lu\n", g_disk_state.stats.bytes_written);
    printf("读取错误: %lu\n", g_disk_state.stats.read_errors);
    printf("写入错误: %lu\n", g_disk_state.stats.write_errors);
    printf("平均读取时间: %.6f 秒\n", g_disk_state.stats.avg_read_time);
    printf("平均写入时间: %.6f 秒\n", g_disk_state.stats.avg_write_time);
    
    if (g_disk_state.stats.last_operation_time > 0) {
        printf("最后操作时间: %s", ctime(&g_disk_state.stats.last_operation_time));
    }
    
    printf("最后同步时间: %s", ctime(&g_disk_state.last_sync_time));
    printf("====================\n\n");
}

/*==============================================================================
 * 块I/O便利函数
 *============================================================================*/

/**
 * 写入多个连续块
 */
int disk_write_blocks(int start_block, int block_count, const char* data) {
    if (!data || block_count <= 0) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < block_count; i++) {
        int result = disk_write_block(start_block + i, 
                                    data + (i * DISK_BLOCK_SIZE));
        if (result != DISK_SUCCESS) {
            return result;
        }
    }
    
    return DISK_SUCCESS;
}

/**
 * 读取多个连续块
 */
int disk_read_blocks(int start_block, int block_count, char* buffer) {
    if (!buffer || block_count <= 0) {
        return DISK_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < block_count; i++) {
        int result = disk_read_block(start_block + i, 
                                   buffer + (i * DISK_BLOCK_SIZE));
        if (result != DISK_SUCCESS) {
            return result;
        }
    }
    
    return DISK_SUCCESS;
}

/**
 * 清零一个块
 */
int disk_zero_block(int block_num) {
    char zero_block[DISK_BLOCK_SIZE];
    memset(zero_block, 0, DISK_BLOCK_SIZE);
    return disk_write_block(block_num, zero_block);
}

/**
 * 复制块数据
 */
int disk_copy_block(int src_block, int dst_block) {
    char buffer[DISK_BLOCK_SIZE];
    
    int result = disk_read_block(src_block, buffer);
    if (result != DISK_SUCCESS) {
        return result;
    }
    
    return disk_write_block(dst_block, buffer);
} 