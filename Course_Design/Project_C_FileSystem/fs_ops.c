/**
 * File System Operations Implementation
 * fs_ops.c
 * 
 * 实现文件系统核心操作，包括格式化、挂载、同步等功能。
 * 
 * 功能特性：
 * - 完整的文件系统格式化流程
 * - 超级块和位图管理
 * - 根目录初始化
 * - 数据完整性保护
 */

#include "fs_ops.h"
#include "user_manager.h"
#include <string.h>
#include <assert.h>

/*==============================================================================
 * 全局文件系统状态
 *============================================================================*/

fs_state_t g_fs_state = {0};

// 文件操作标志位定义
#define FS_OPEN_READ        0x01        // 读模式
#define FS_OPEN_WRITE       0x02        // 写模式
#define FS_OPEN_APPEND      0x04        // 追加模式
#define FS_OPEN_CREATE      0x08        // 创建文件（如果不存在）
#define FS_OPEN_TRUNCATE    0x10        // 截断文件
#define FS_OPEN_EXCL        0x20        // 排他性创建

/*==============================================================================
 * 工具函数实现
 *============================================================================*/

/**
 * 计算校验和 - 简单的CRC32实现
 */
uint32_t fs_ops_calculate_checksum(const void *data, size_t size) {
    if (!data || size == 0) {
        return 0;
    }
    
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t checksum = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        checksum ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 1) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum >>= 1;
            }
        }
    }
    
    return ~checksum;
}

/**
 * 获取当前时间
 */
time_t fs_ops_current_time(void) {
    return time(NULL);
}

/**
 * 错误码转换为字符串
 */
const char* fs_ops_error_to_string(fs_error_t error) {
    switch (error) {
        case FS_SUCCESS:              return "操作成功";
        case FS_ERROR_INVALID_PARAM:  return "无效参数";
        case FS_ERROR_NO_MEMORY:      return "内存不足";
        case FS_ERROR_NO_SPACE:       return "磁盘空间不足";
        case FS_ERROR_FILE_NOT_FOUND: return "文件或目录不存在";
        case FS_ERROR_FILE_EXISTS:    return "文件已存在";
        case FS_ERROR_NOT_DIRECTORY:  return "不是目录";
        case FS_ERROR_IS_DIRECTORY:   return "是目录";
        case FS_ERROR_PERMISSION:     return "权限不足";
        case FS_ERROR_FILE_OPEN:      return "文件正在使用";
        case FS_ERROR_TOO_MANY_OPEN:  return "打开文件过多";
        case FS_ERROR_IO:             return "I/O错误";
        case FS_ERROR_CORRUPTED:      return "文件系统损坏";
        case FS_ERROR_NOT_MOUNTED:    return "文件系统未挂载";
        case FS_ERROR_ALREADY_MOUNTED: return "文件系统已挂载";
        default:                      return "未知错误";
    }
}

/*==============================================================================
 * 超级块管理
 *============================================================================*/

/**
 * 初始化超级块
 */
fs_error_t fs_ops_init_superblock(fs_superblock_t *sb, uint32_t total_blocks) {
    if (!sb) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 清零超级块结构
    memset(sb, 0, sizeof(fs_superblock_t));
    
    // 文件系统标识信息
    sb->magic_number = FS_MAGIC_NUMBER;
    sb->version = 1;
    sb->block_size = BLOCK_SIZE;
    
    // 计算文件系统布局
    sb->total_blocks = total_blocks;
    sb->total_inodes = FS_DEFAULT_MAX_INODES;
    
    // 计算各个区域的位置
    sb->inode_table_start = FS_INODE_TABLE_START;
    sb->inode_table_blocks = (sb->total_inodes * sizeof(fs_inode_t) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sb->data_blocks_start = sb->inode_table_start + sb->inode_table_blocks;
    
    // 初始化空闲计数（稍后会在位图初始化时更新）
    sb->free_blocks = total_blocks - sb->data_blocks_start;
    sb->free_inodes = sb->total_inodes - 1; // 减去根目录inode
    
    // 设置根目录inode号
    sb->root_inode = ROOT_INODE_NUM;
    
    // 时间戳
    time_t current_time = fs_ops_current_time();
    sb->created_time = current_time;
    sb->last_mount_time = current_time;
    sb->last_write_time = current_time;
    sb->last_check_time = current_time;
    
    // 挂载计数
    sb->mount_count = 0;
    sb->max_mount_count = 100;
    
    // 计算校验和（不包括校验和字段本身）
    sb->checksum = 0;
    sb->checksum = fs_ops_calculate_checksum(sb, offsetof(fs_superblock_t, checksum));
    
    printf("超级块初始化完成:\n");
    printf("  总块数: %u\n", sb->total_blocks);
    printf("  总inode数: %u\n", sb->total_inodes);
    printf("  inode表起始: %u\n", sb->inode_table_start);
    printf("  数据块起始: %u\n", sb->data_blocks_start);
    printf("  可用块数: %u\n", sb->free_blocks);
    
    return FS_SUCCESS;
}

/**
 * 写入超级块到磁盘
 */
fs_error_t fs_ops_write_superblock(const fs_superblock_t *sb) {
    if (!sb) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    char buffer[DISK_BLOCK_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    // 将超级块复制到缓冲区
    memcpy(buffer, sb, sizeof(fs_superblock_t));
    
    // 写入磁盘第0块
    int result = disk_write_block(FS_SUPERBLOCK_BLOCK, buffer);
    if (result != DISK_SUCCESS) {
        printf("写入超级块失败: %s\n", disk_error_to_string(result));
        return FS_ERROR_IO;
    }
    
    printf("超级块已写入磁盘块 %d\n", FS_SUPERBLOCK_BLOCK);
    return FS_SUCCESS;
}

/**
 * 从磁盘读取超级块
 */
fs_error_t fs_ops_read_superblock(fs_superblock_t *sb) {
    if (!sb) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    char buffer[DISK_BLOCK_SIZE];
    
    // 从磁盘第0块读取
    int result = disk_read_block(FS_SUPERBLOCK_BLOCK, buffer);
    if (result != DISK_SUCCESS) {
        printf("读取超级块失败: %s\n", disk_error_to_string(result));
        return FS_ERROR_IO;
    }
    
    // 复制到超级块结构
    memcpy(sb, buffer, sizeof(fs_superblock_t));
    
    // 验证魔数
    if (sb->magic_number != FS_MAGIC_NUMBER) {
        printf("无效的文件系统魔数: 0x%x\n", sb->magic_number);
        return FS_ERROR_CORRUPTED;
    }
    
    // 验证校验和
    uint32_t stored_checksum = sb->checksum;
    sb->checksum = 0;
    uint32_t calculated_checksum = fs_ops_calculate_checksum(sb, offsetof(fs_superblock_t, checksum));
    sb->checksum = stored_checksum;
    
    if (stored_checksum != calculated_checksum) {
        printf("超级块校验和不匹配: 存储值=0x%x, 计算值=0x%x\n", 
               stored_checksum, calculated_checksum);
        return FS_ERROR_CORRUPTED;
    }
    
    printf("超级块验证成功\n");
    return FS_SUCCESS;
}

/*==============================================================================
 * 位图管理
 *============================================================================*/

/**
 * 初始化位图
 */
fs_error_t fs_ops_init_bitmap(fs_bitmap_t *bitmap, uint32_t total_bits) {
    if (!bitmap || total_bits == 0) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 计算需要的字节数
    uint32_t bitmap_bytes = (total_bits + 7) / 8;
    
    // 分配内存
    bitmap->bitmap = (uint8_t *)calloc(bitmap_bytes, 1);
    if (!bitmap->bitmap) {
        return FS_ERROR_NO_MEMORY;
    }
    
    // 初始化位图结构
    bitmap->total_bits = total_bits;
    bitmap->free_count = total_bits;
    bitmap->last_allocated = 0;
    
    printf("位图初始化完成: %u 位，%u 字节\n", total_bits, bitmap_bytes);
    return FS_SUCCESS;
}

/**
 * 设置位图中的位
 */
static void set_bitmap_bit(fs_bitmap_t *bitmap, uint32_t bit_num) {
    if (!bitmap || !bitmap->bitmap || bit_num >= bitmap->total_bits) {
        return;
    }
    
    uint32_t byte_index = bit_num / 8;
    uint32_t bit_offset = bit_num % 8;
    
    if (!(bitmap->bitmap[byte_index] & (1 << bit_offset))) {
        bitmap->bitmap[byte_index] |= (1 << bit_offset);
        bitmap->free_count--;
    }
}

/**
 * 写入位图到磁盘
 */
fs_error_t fs_ops_write_bitmap(const fs_bitmap_t *bitmap, uint32_t start_block, uint32_t block_count) {
    if (!bitmap || !bitmap->bitmap) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    uint32_t bitmap_bytes = (bitmap->total_bits + 7) / 8;
    uint32_t bytes_per_block = DISK_BLOCK_SIZE;
    uint32_t total_bytes_needed = (bitmap_bytes + bytes_per_block - 1) / bytes_per_block * bytes_per_block;
    
    // 确保有足够的块来存储位图
    if (total_bytes_needed > block_count * bytes_per_block) {
        printf("位图太大，需要 %u 字节，但只有 %u 块可用\n", 
               total_bytes_needed, block_count);
        return FS_ERROR_NO_SPACE;
    }
    
    char buffer[DISK_BLOCK_SIZE];
    uint32_t bytes_written = 0;
    
    for (uint32_t block = 0; block < block_count && bytes_written < bitmap_bytes; block++) {
        memset(buffer, 0, sizeof(buffer));
        
        uint32_t bytes_to_copy = (bitmap_bytes - bytes_written);
        if (bytes_to_copy > bytes_per_block) {
            bytes_to_copy = bytes_per_block;
        }
        
        memcpy(buffer, bitmap->bitmap + bytes_written, bytes_to_copy);
        
        int result = disk_write_block(start_block + block, buffer);
        if (result != DISK_SUCCESS) {
            printf("写入位图块 %u 失败: %s\n", 
                   start_block + block, disk_error_to_string(result));
            return FS_ERROR_IO;
        }
        
        bytes_written += bytes_to_copy;
    }
    
    printf("位图已写入磁盘，起始块: %u，块数: %u\n", start_block, block_count);
    return FS_SUCCESS;
}

/**
 * 从磁盘读取位图
 */
fs_error_t fs_ops_read_bitmap(fs_bitmap_t *bitmap, uint32_t start_block, uint32_t block_count) {
    if (!bitmap) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    uint32_t bitmap_bytes = (bitmap->total_bits + 7) / 8;
    uint32_t bytes_per_block = DISK_BLOCK_SIZE;
    char buffer[DISK_BLOCK_SIZE];
    uint32_t bytes_read = 0;
    
    for (uint32_t block = 0; block < block_count && bytes_read < bitmap_bytes; block++) {
        int result = disk_read_block(start_block + block, buffer);
        if (result != DISK_SUCCESS) {
            printf("读取位图块 %u 失败: %s\n", 
                   start_block + block, disk_error_to_string(result));
            return FS_ERROR_IO;
        }
        
        uint32_t bytes_to_copy = (bitmap_bytes - bytes_read);
        if (bytes_to_copy > bytes_per_block) {
            bytes_to_copy = bytes_per_block;
        }
        
        memcpy(bitmap->bitmap + bytes_read, buffer, bytes_to_copy);
        bytes_read += bytes_to_copy;
    }
    
    // 重新计算空闲计数
    bitmap->free_count = 0;
    for (uint32_t i = 0; i < bitmap->total_bits; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_offset = i % 8;
        if (!(bitmap->bitmap[byte_index] & (1 << bit_offset))) {
            bitmap->free_count++;
        }
    }
    
    printf("位图已从磁盘读取，空闲计数: %u\n", bitmap->free_count);
    return FS_SUCCESS;
}

/*==============================================================================
 * 根目录创建
 *============================================================================*/

/**
 * 创建根目录
 */
fs_error_t fs_ops_create_root_directory(void) {
    printf("开始创建根目录...\n");
    
    // 1. 初始化根目录的inode
    fs_inode_t root_inode;
    memset(&root_inode, 0, sizeof(fs_inode_t));
    
    // 设置inode属性
    root_inode.inode_number = ROOT_INODE_NUM;
    root_inode.file_type = FS_FILE_TYPE_DIRECTORY;
    root_inode.permissions = FS_ROOT_PERMISSIONS;
    root_inode.owner_uid = FS_ROOT_UID;
    root_inode.owner_gid = FS_ROOT_GID;
    root_inode.link_count = 2; // "." 和来自父目录的链接
    
    // 时间戳
    time_t current_time = fs_ops_current_time();
    root_inode.access_time = current_time;
    root_inode.modify_time = current_time;
    root_inode.change_time = current_time;
    root_inode.create_time = current_time;
    
    // 2. 分配一个数据块用于存储目录项
    uint32_t data_block = g_fs_state.superblock.data_blocks_start; // 使用第一个数据块
    root_inode.direct_blocks[0] = data_block;
    root_inode.block_count = 1;
    
    // 3. 创建目录项数据
    char dir_block[DISK_BLOCK_SIZE];
    memset(dir_block, 0, sizeof(dir_block));
    
    fs_dir_entry_t *entries = (fs_dir_entry_t *)dir_block;
    
    // 创建 "." 目录项（指向自己）
    entries[0].inode_number = ROOT_INODE_NUM;
    entries[0].entry_length = sizeof(fs_dir_entry_t);
    entries[0].filename_length = 1;
    entries[0].file_type = FS_FILE_TYPE_DIRECTORY;
    strcpy(entries[0].filename, ".");
    entries[0].is_valid = 1;
    
    // 创建 ".." 目录项（根目录的父目录就是自己）
    entries[1].inode_number = ROOT_INODE_NUM;
    entries[1].entry_length = sizeof(fs_dir_entry_t);
    entries[1].filename_length = 2;
    entries[1].file_type = FS_FILE_TYPE_DIRECTORY;
    strcpy(entries[1].filename, "..");
    entries[1].is_valid = 1;
    
    // 计算目录大小
    root_inode.file_size = 2 * sizeof(fs_dir_entry_t);
    
    // 4. 将目录数据写入磁盘
    int result = disk_write_block(data_block, dir_block);
    if (result != DISK_SUCCESS) {
        printf("写入根目录数据块失败: %s\n", disk_error_to_string(result));
        return FS_ERROR_IO;
    }
    
    // 5. 将根目录inode写入inode表
    char inode_block[DISK_BLOCK_SIZE];
    memset(inode_block, 0, sizeof(inode_block));
    
    // 计算根目录inode在inode表中的位置
    uint32_t inodes_per_block = DISK_BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = g_fs_state.superblock.inode_table_start + 
                              (ROOT_INODE_NUM / inodes_per_block);
    uint32_t inode_offset = (ROOT_INODE_NUM % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取inode块（可能已有其他inode）
    result = disk_read_block(inode_block_num, inode_block);
    if (result != DISK_SUCCESS) {
        // 如果读取失败，说明是新块，继续使用空白块
        memset(inode_block, 0, sizeof(inode_block));
    }
    
    // 将根目录inode复制到正确位置
    memcpy(inode_block + inode_offset, &root_inode, sizeof(fs_inode_t));
    
    // 写回inode块
    result = disk_write_block(inode_block_num, inode_block);
    if (result != DISK_SUCCESS) {
        printf("写入根目录inode失败: %s\n", disk_error_to_string(result));
        return FS_ERROR_IO;
    }
    
    // 6. 更新位图：标记根目录inode和数据块为已使用
    set_bitmap_bit(&g_fs_state.inode_bitmap, ROOT_INODE_NUM);
    set_bitmap_bit(&g_fs_state.block_bitmap, data_block - g_fs_state.superblock.data_blocks_start);
    
    printf("根目录创建成功:\n");
    printf("  inode号: %u\n", ROOT_INODE_NUM);
    printf("  数据块: %u\n", data_block);
    printf("  权限: 0%o\n", FS_ROOT_PERMISSIONS);
    printf("  大小: %lu 字节\n", root_inode.file_size);
    
    return FS_SUCCESS;
}

/*==============================================================================
 * 主要格式化函数
 *============================================================================*/

/**
 * 格式化磁盘 - 主函数
 */
void format_disk(void) {
    printf("==================== 开始格式化文件系统 ====================\n");
    
    // 1. 检查磁盘是否已初始化
    if (!disk_is_initialized()) {
        printf("错误：磁盘未初始化，请先调用 disk_init()\n");
        return;
    }
    
    // 获取磁盘信息
    uint32_t total_blocks, block_size;
    uint64_t disk_size;
    int result = disk_get_info(&total_blocks, &block_size, &disk_size);
    if (result != DISK_SUCCESS) {
        printf("错误：无法获取磁盘信息: %s\n", disk_error_to_string(result));
        return;
    }
    
    printf("磁盘信息:\n");
    printf("  总块数: %u\n", total_blocks);
    printf("  块大小: %u 字节\n", block_size);
    printf("  磁盘大小: %lu 字节\n", disk_size);
    
    // 验证块大小
    if (block_size != BLOCK_SIZE) {
        printf("错误：块大小不匹配，期望 %d，实际 %u\n", BLOCK_SIZE, block_size);
        return;
    }
    
    // 2. 初始化超级块
    printf("\n步骤 1: 初始化超级块...\n");
    fs_error_t fs_result = fs_ops_init_superblock(&g_fs_state.superblock, total_blocks);
    if (fs_result != FS_SUCCESS) {
        printf("错误：初始化超级块失败: %s\n", fs_ops_error_to_string(fs_result));
        return;
    }
    
    // 3. 写入超级块到磁盘
    printf("\n步骤 2: 写入超级块到磁盘...\n");
    fs_result = fs_ops_write_superblock(&g_fs_state.superblock);
    if (fs_result != FS_SUCCESS) {
        printf("错误：写入超级块失败: %s\n", fs_ops_error_to_string(fs_result));
        return;
    }
    
    // 4. 初始化inode位图
    printf("\n步骤 3: 初始化inode位图...\n");
    fs_result = fs_ops_init_bitmap(&g_fs_state.inode_bitmap, g_fs_state.superblock.total_inodes);
    if (fs_result != FS_SUCCESS) {
        printf("错误：初始化inode位图失败: %s\n", fs_ops_error_to_string(fs_result));
        return;
    }
    
    // 5. 初始化数据块位图
    printf("\n步骤 4: 初始化数据块位图...\n");
    uint32_t data_blocks_count = total_blocks - g_fs_state.superblock.data_blocks_start;
    fs_result = fs_ops_init_bitmap(&g_fs_state.block_bitmap, data_blocks_count);
    if (fs_result != FS_SUCCESS) {
        printf("错误：初始化数据块位图失败: %s\n", fs_ops_error_to_string(fs_result));
        return;
    }
    
    // 6. 创建根目录（这会标记inode和数据块为已使用）
    printf("\n步骤 5: 创建根目录...\n");
    fs_result = fs_ops_create_root_directory();
    if (fs_result != FS_SUCCESS) {
        printf("错误：创建根目录失败: %s\n", fs_ops_error_to_string(fs_result));
        goto cleanup;
    }
    
    // 7. 写入inode位图到磁盘（此时已包含根目录inode标记）
    printf("\n步骤 6: 写入inode位图到磁盘...\n");
    fs_result = fs_ops_write_bitmap(&g_fs_state.inode_bitmap, 
                                   FS_INODE_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
    if (fs_result != FS_SUCCESS) {
        printf("错误：写入inode位图失败: %s\n", fs_ops_error_to_string(fs_result));
        goto cleanup;
    }
    
    // 8. 写入数据块位图到磁盘（此时已包含根目录数据块标记）
    printf("\n步骤 7: 写入数据块位图到磁盘...\n");
    fs_result = fs_ops_write_bitmap(&g_fs_state.block_bitmap, 
                                   FS_DATA_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
    if (fs_result != FS_SUCCESS) {
        printf("错误：写入数据块位图失败: %s\n", fs_ops_error_to_string(fs_result));
        goto cleanup;
    }
    
    // 9. 更新超级块中的空闲计数
    printf("\n步骤 8: 更新空闲计数...\n");
    g_fs_state.superblock.free_inodes = g_fs_state.inode_bitmap.free_count;
    g_fs_state.superblock.free_blocks = g_fs_state.block_bitmap.free_count;
    
    // 重新计算超级块校验和并写入
    g_fs_state.superblock.checksum = 0;
    g_fs_state.superblock.checksum = fs_ops_calculate_checksum(&g_fs_state.superblock, 
                                                              offsetof(fs_superblock_t, checksum));
    
    fs_result = fs_ops_write_superblock(&g_fs_state.superblock);
    if (fs_result != FS_SUCCESS) {
        printf("错误：更新超级块失败: %s\n", fs_ops_error_to_string(fs_result));
        goto cleanup;
    }
    
    // 10. 同步数据到磁盘
    printf("\n步骤 9: 同步数据到磁盘...\n");
    result = disk_sync();
    if (result != DISK_SUCCESS) {
        printf("警告：同步磁盘失败: %s\n", disk_error_to_string(result));
    }
    
    printf("\n==================== 文件系统格式化完成 ====================\n");
    printf("文件系统统计:\n");
    printf("  总inode数: %u (可用: %u)\n", 
           g_fs_state.superblock.total_inodes, g_fs_state.superblock.free_inodes);
    printf("  总数据块数: %u (可用: %u)\n", 
           data_blocks_count, g_fs_state.superblock.free_blocks);
    printf("  文件系统大小: %.2f MB\n", disk_size / (1024.0 * 1024.0));
    printf("  可用空间: %.2f MB\n", 
           (g_fs_state.superblock.free_blocks * BLOCK_SIZE) / (1024.0 * 1024.0));
    printf("============================================================\n");
    
    return;

cleanup:
    // 清理分配的内存
    if (g_fs_state.inode_bitmap.bitmap) {
        free(g_fs_state.inode_bitmap.bitmap);
        g_fs_state.inode_bitmap.bitmap = NULL;
    }
    if (g_fs_state.block_bitmap.bitmap) {
        free(g_fs_state.block_bitmap.bitmap);
        g_fs_state.block_bitmap.bitmap = NULL;
    }
}

/*==============================================================================
 * 状态打印函数
 *============================================================================*/

/**
 * 打印文件系统状态
 */
void fs_ops_print_status(void) {
    printf("==================== 文件系统状态 ====================\n");
    printf("超级块信息:\n");
    printf("  魔数: 0x%x\n", g_fs_state.superblock.magic_number);
    printf("  版本: %u\n", g_fs_state.superblock.version);
    printf("  块大小: %u 字节\n", g_fs_state.superblock.block_size);
    printf("  总块数: %u\n", g_fs_state.superblock.total_blocks);
    printf("  总inode数: %u\n", g_fs_state.superblock.total_inodes);
    printf("  可用块数: %u\n", g_fs_state.superblock.free_blocks);
    printf("  可用inode数: %u\n", g_fs_state.superblock.free_inodes);
    printf("  根inode: %u\n", g_fs_state.superblock.root_inode);
    
    if (g_fs_state.inode_bitmap.bitmap) {
        printf("\ninode位图:\n");
        printf("  总位数: %u\n", g_fs_state.inode_bitmap.total_bits);
        printf("  空闲数: %u\n", g_fs_state.inode_bitmap.free_count);
    }
    
    if (g_fs_state.block_bitmap.bitmap) {
        printf("\n数据块位图:\n");
        printf("  总位数: %u\n", g_fs_state.block_bitmap.total_bits);
        printf("  空闲数: %u\n", g_fs_state.block_bitmap.free_count);
    }
    
    printf("=====================================================\n");
}

/*==============================================================================
 * 文件操作函数前向声明
 *============================================================================*/

fs_error_t fs_ops_read_inode(uint32_t inode_number, fs_inode_t *inode);
fs_error_t fs_ops_write_inode(uint32_t inode_number, const fs_inode_t *inode);

/*==============================================================================
 * 文件操作函数实现
 *============================================================================*/

/**
 * 分配位图中的位
 */
static uint32_t alloc_bitmap_bit(fs_bitmap_t *bitmap) {
    if (!bitmap || !bitmap->bitmap || bitmap->free_count == 0) {
        return 0; // 没有可用的位
    }
    
    // 从上次分配的位置开始搜索
    uint32_t start_bit = bitmap->last_allocated;
    
    for (uint32_t i = 0; i < bitmap->total_bits; i++) {
        uint32_t bit_num = (start_bit + i) % bitmap->total_bits;
        uint32_t byte_index = bit_num / 8;
        uint32_t bit_offset = bit_num % 8;
        
        // 检查这个位是否空闲
        if (!(bitmap->bitmap[byte_index] & (1 << bit_offset))) {
            // 设置这个位为已使用
            bitmap->bitmap[byte_index] |= (1 << bit_offset);
            bitmap->free_count--;
            bitmap->last_allocated = bit_num;
            return bit_num;
        }
    }
    
    return 0; // 没有找到空闲的位
}

/**
 * 释放位图中的位
 */
static void free_bitmap_bit(fs_bitmap_t *bitmap, uint32_t bit_num) {
    if (!bitmap || !bitmap->bitmap || bit_num >= bitmap->total_bits) {
        return;
    }
    
    uint32_t byte_index = bit_num / 8;
    uint32_t bit_offset = bit_num % 8;
    
    // 只有当位当前是已分配状态时才释放
    if (bitmap->bitmap[byte_index] & (1 << bit_offset)) {
        bitmap->bitmap[byte_index] &= ~(1 << bit_offset);
        bitmap->free_count++;
    }
}

/**
 * 路径解析 - 简化版本，只支持单层文件名
 */
static fs_error_t parse_path(const char *path, uint32_t *parent_inode, char *filename) {
    if (!path || !parent_inode || !filename) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 简化实现：假设路径就是文件名，父目录是当前目录或根目录
    if (path[0] == '/') {
        // 绝对路径，从根目录开始
        *parent_inode = ROOT_INODE_NUM;
        
        // 跳过开头的斜杠
        const char *name_start = path + 1;
        if (strlen(name_start) == 0) {
            // 路径是根目录
            strcpy(filename, "");
            return FS_SUCCESS;
        }
        
        // 检查是否包含更多的斜杠（多级路径暂不支持）
        if (strchr(name_start, '/') != NULL) {
            return FS_ERROR_INVALID_PARAM; // 多级路径暂不支持
        }
        
        strncpy(filename, name_start, MAX_FILENAME_LEN - 1);
        filename[MAX_FILENAME_LEN - 1] = '\0';
    } else {
        // 相对路径，从当前目录开始（简化为根目录）
        *parent_inode = ROOT_INODE_NUM;
        
        // 检查是否包含斜杠
        if (strchr(path, '/') != NULL) {
            return FS_ERROR_INVALID_PARAM; // 多级路径暂不支持
        }
        
        strncpy(filename, path, MAX_FILENAME_LEN - 1);
        filename[MAX_FILENAME_LEN - 1] = '\0';
    }
    
    return FS_SUCCESS;
}

/**
 * 在目录中查找文件
 */
static uint32_t find_file_in_directory(uint32_t dir_inode_num, const char *filename) {
    if (!filename || dir_inode_num == 0) {
        return 0;
    }
    
    // 读取目录inode
    fs_inode_t dir_inode;
    if (fs_ops_read_inode(dir_inode_num, &dir_inode) != FS_SUCCESS) {
        return 0;
    }
    
    // 确保是目录
    if (dir_inode.file_type != FS_FILE_TYPE_DIRECTORY) {
        return 0;
    }
    
    // 遍历目录的数据块查找文件
    for (uint32_t block_idx = 0; block_idx < DIRECT_BLOCKS && dir_inode.direct_blocks[block_idx] != 0; block_idx++) {
        char block_data[DISK_BLOCK_SIZE];
        int result = disk_read_block(dir_inode.direct_blocks[block_idx], block_data);
        if (result != DISK_SUCCESS) {
            continue;
        }
        
        // 遍历目录项
        fs_dir_entry_t *entries = (fs_dir_entry_t *)block_data;
        uint32_t max_entries = DISK_BLOCK_SIZE / sizeof(fs_dir_entry_t);
        
        for (uint32_t i = 0; i < max_entries; i++) {
            if (entries[i].is_valid && strcmp(entries[i].filename, filename) == 0) {
                return entries[i].inode_number;
            }
        }
    }
    
    return 0; // 未找到
}

/**
 * 向目录中添加文件项
 */
static fs_error_t add_file_to_directory(uint32_t dir_inode_num, const char *filename, uint32_t file_inode_num) {
    if (!filename || dir_inode_num == 0 || file_inode_num == 0) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 读取目录inode
    fs_inode_t dir_inode;
    if (fs_ops_read_inode(dir_inode_num, &dir_inode) != FS_SUCCESS) {
        return FS_ERROR_IO;
    }
    
    // 确保是目录
    if (dir_inode.file_type != FS_FILE_TYPE_DIRECTORY) {
        return FS_ERROR_NOT_DIRECTORY;
    }
    
    // 查找空闲的目录项位置
    for (uint32_t block_idx = 0; block_idx < DIRECT_BLOCKS; block_idx++) {
        char block_data[DISK_BLOCK_SIZE];
        
        if (dir_inode.direct_blocks[block_idx] == 0) {
            // 需要分配新的数据块
            uint32_t new_block = alloc_bitmap_bit(&g_fs_state.block_bitmap);
            if (new_block == 0) {
                return FS_ERROR_NO_SPACE;
            }
            
            // 转换为绝对块号
            new_block += g_fs_state.superblock.data_blocks_start;
            dir_inode.direct_blocks[block_idx] = new_block;
            dir_inode.block_count++;
            
            // 清空新块
            memset(block_data, 0, sizeof(block_data));
        } else {
            // 读取现有块
            int result = disk_read_block(dir_inode.direct_blocks[block_idx], block_data);
            if (result != DISK_SUCCESS) {
                return FS_ERROR_IO;
            }
        }
        
        // 查找空闲的目录项
        fs_dir_entry_t *entries = (fs_dir_entry_t *)block_data;
        uint32_t max_entries = DISK_BLOCK_SIZE / sizeof(fs_dir_entry_t);
        
        for (uint32_t i = 0; i < max_entries; i++) {
            if (!entries[i].is_valid) {
                // 找到空闲项，添加新文件
                entries[i].inode_number = file_inode_num;
                entries[i].entry_length = sizeof(fs_dir_entry_t);
                entries[i].filename_length = strlen(filename);
                entries[i].file_type = FS_FILE_TYPE_REGULAR; // 默认为普通文件
                strncpy(entries[i].filename, filename, MAX_FILENAME_LEN - 1);
                entries[i].filename[MAX_FILENAME_LEN - 1] = '\0';
                entries[i].is_valid = 1;
                
                // 写回数据块
                int result = disk_write_block(dir_inode.direct_blocks[block_idx], block_data);
                if (result != DISK_SUCCESS) {
                    return FS_ERROR_IO;
                }
                
                // 更新目录大小
                dir_inode.file_size = (block_idx + 1) * DISK_BLOCK_SIZE;
                
                // 更新目录inode的时间戳
                time_t current_time = fs_ops_current_time();
                dir_inode.modify_time = current_time;
                dir_inode.change_time = current_time;
                
                // 写回目录inode
                fs_ops_write_inode(dir_inode_num, &dir_inode);
                
                return FS_SUCCESS;
            }
        }
    }
    
    return FS_ERROR_NO_SPACE; // 目录已满
}

/**
 * 读取inode
 */
fs_error_t fs_ops_read_inode(uint32_t inode_number, fs_inode_t *inode) {
    if (!inode || inode_number == 0 || inode_number >= g_fs_state.superblock.total_inodes) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 计算inode在inode表中的位置
    uint32_t inodes_per_block = DISK_BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = g_fs_state.superblock.inode_table_start + (inode_number / inodes_per_block);
    uint32_t inode_offset = (inode_number % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取inode块
    char inode_block[DISK_BLOCK_SIZE];
    int result = disk_read_block(inode_block_num, inode_block);
    if (result != DISK_SUCCESS) {
        return FS_ERROR_IO;
    }
    
    // 复制inode数据
    memcpy(inode, inode_block + inode_offset, sizeof(fs_inode_t));
    
    return FS_SUCCESS;
}

/**
 * 写入inode
 */
fs_error_t fs_ops_write_inode(uint32_t inode_number, const fs_inode_t *inode) {
    if (!inode || inode_number == 0 || inode_number >= g_fs_state.superblock.total_inodes) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 计算inode在inode表中的位置
    uint32_t inodes_per_block = DISK_BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = g_fs_state.superblock.inode_table_start + (inode_number / inodes_per_block);
    uint32_t inode_offset = (inode_number % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取inode块
    char inode_block[DISK_BLOCK_SIZE];
    int result = disk_read_block(inode_block_num, inode_block);
    if (result != DISK_SUCCESS) {
        return FS_ERROR_IO;
    }
    
    // 复制inode数据
    memcpy(inode_block + inode_offset, inode, sizeof(fs_inode_t));
    
    // 写回inode块
    result = disk_write_block(inode_block_num, inode_block);
    if (result != DISK_SUCCESS) {
        return FS_ERROR_IO;
    }
    
    return FS_SUCCESS;
}

/**
 * 加载文件系统状态
 */
static fs_error_t load_filesystem_state(void) {
    // 读取超级块
    fs_error_t result = fs_ops_read_superblock(&g_fs_state.superblock);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 读取inode位图
    result = fs_ops_read_bitmap(&g_fs_state.inode_bitmap, FS_INODE_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 读取数据块位图
    result = fs_ops_read_bitmap(&g_fs_state.block_bitmap, FS_DATA_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 初始化文件句柄表
    memset(g_fs_state.open_files, 0, sizeof(g_fs_state.open_files));
    
    printf("文件系统状态加载完成\n");
    return FS_SUCCESS;
}

/**
 * 创建文件
 */
int fs_create(const char* path) {
    if (!path) {
        printf("错误：路径参数无效\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 确保文件系统状态已加载
    if (g_fs_state.superblock.magic_number != FS_MAGIC_NUMBER) {
        fs_error_t result = load_filesystem_state();
        if (result != FS_SUCCESS) {
            printf("错误：无法加载文件系统状态: %s\n", fs_ops_error_to_string(result));
            return result;
        }
    }
    
    printf("创建文件: %s\n", path);
    
    // 解析路径
    uint32_t parent_inode;
    char filename[MAX_FILENAME_LEN];
    fs_error_t result = parse_path(path, &parent_inode, filename);
    if (result != FS_SUCCESS) {
        printf("错误：路径解析失败: %s\n", fs_ops_error_to_string(result));
        return result;
    }
    
    if (strlen(filename) == 0) {
        printf("错误：文件名不能为空\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 检查文件是否已存在
    uint32_t existing_inode = find_file_in_directory(parent_inode, filename);
    if (existing_inode != 0) {
        printf("错误：文件已存在\n");
        return FS_ERROR_FILE_EXISTS;
    }
    
    // 分配新的inode
    uint32_t new_inode_num = alloc_bitmap_bit(&g_fs_state.inode_bitmap);
    if (new_inode_num == 0) {
        printf("错误：无法分配inode\n");
        return FS_ERROR_NO_SPACE;
    }
    
    // 创建inode
    fs_inode_t new_inode;
    memset(&new_inode, 0, sizeof(fs_inode_t));
    
    // 设置inode属性
    new_inode.inode_number = new_inode_num;
    new_inode.file_type = FS_FILE_TYPE_REGULAR;
    new_inode.permissions = 0644; // rw-r--r--
    new_inode.owner_uid = user_manager_get_current_uid(); // 使用当前用户ID
    new_inode.owner_gid = user_manager_get_current_gid(); // 使用当前用户组ID
    new_inode.link_count = 1;
    new_inode.file_size = 0;
    new_inode.block_count = 0;
    
    // 设置时间戳
    time_t current_time = fs_ops_current_time();
    new_inode.access_time = current_time;
    new_inode.modify_time = current_time;
    new_inode.change_time = current_time;
    new_inode.create_time = current_time;
    
    // 写入inode到磁盘
    result = fs_ops_write_inode(new_inode_num, &new_inode);
    if (result != FS_SUCCESS) {
        // 回滚：释放已分配的inode
        free_bitmap_bit(&g_fs_state.inode_bitmap, new_inode_num);
        printf("错误：写入inode失败: %s\n", fs_ops_error_to_string(result));
        return result;
    }
    
    // 将文件添加到父目录
    result = add_file_to_directory(parent_inode, filename, new_inode_num);
    if (result != FS_SUCCESS) {
        // 回滚：释放已分配的inode
        free_bitmap_bit(&g_fs_state.inode_bitmap, new_inode_num);
        printf("错误：添加到目录失败: %s\n", fs_ops_error_to_string(result));
        return result;
    }
    
    // 更新超级块中的空闲inode计数
    g_fs_state.superblock.free_inodes = g_fs_state.inode_bitmap.free_count;
    
    printf("文件创建成功: %s (inode: %u)\n", path, new_inode_num);
    return FS_SUCCESS;
}

/**
 * 打开文件
 */
int fs_open(const char* path) {
    if (!path) {
        printf("错误：路径参数无效\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 确保文件系统状态已加载
    if (g_fs_state.superblock.magic_number != FS_MAGIC_NUMBER) {
        fs_error_t result = load_filesystem_state();
        if (result != FS_SUCCESS) {
            printf("错误：无法加载文件系统状态: %s\n", fs_ops_error_to_string(result));
            return result;
        }
    }
    
    printf("打开文件: %s\n", path);
    
    // 解析路径
    uint32_t parent_inode;
    char filename[MAX_FILENAME_LEN];
    fs_error_t result = parse_path(path, &parent_inode, filename);
    if (result != FS_SUCCESS) {
        printf("错误：路径解析失败: %s\n", fs_ops_error_to_string(result));
        return result;
    }
    
    if (strlen(filename) == 0) {
        printf("错误：文件名不能为空\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 查找文件
    uint32_t file_inode_num = find_file_in_directory(parent_inode, filename);
    if (file_inode_num == 0) {
        printf("错误：文件不存在\n");
        return FS_ERROR_FILE_NOT_FOUND;
    }
    
    // 读取文件inode
    fs_inode_t file_inode;
    result = fs_ops_read_inode(file_inode_num, &file_inode);
    if (result != FS_SUCCESS) {
        printf("错误：读取文件inode失败: %s\n", fs_ops_error_to_string(result));
        return result;
    }
    
    // 确保不是目录
    if (file_inode.file_type == FS_FILE_TYPE_DIRECTORY) {
        printf("错误：试图打开目录作为文件\n");
        return FS_ERROR_IS_DIRECTORY;
    }
    
    // 权限检查：检查当前用户是否有读权限
    if (!user_manager_check_permission(&file_inode, FS_PERM_OWNER_READ)) {
        printf("错误：权限不足 - 无法读取文件\n");
        return FS_ERROR_PERMISSION;
    }
    
    // 查找空闲的文件描述符
    for (int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (g_fs_state.open_files[fd].reference_count == 0) {
            // 初始化文件句柄
            g_fs_state.open_files[fd].inode_number = file_inode_num;
            g_fs_state.open_files[fd].flags = FS_OPEN_READ | FS_OPEN_WRITE; // 默认读写权限
            g_fs_state.open_files[fd].file_position = 0;
            g_fs_state.open_files[fd].reference_count = 1;
            g_fs_state.open_files[fd].open_time = fs_ops_current_time();
            g_fs_state.open_files[fd].owner_uid = user_manager_get_current_uid(); // 使用当前用户ID
            
            // 更新文件访问时间
            file_inode.access_time = fs_ops_current_time();
            fs_ops_write_inode(file_inode_num, &file_inode);
            
            printf("文件打开成功: %s (fd: %d, inode: %u)\n", path, fd, file_inode_num);
            return fd; // 返回文件描述符
        }
    }
    
    printf("错误：打开的文件太多\n");
    return FS_ERROR_TOO_MANY_OPEN;
}

/**
 * 关闭文件
 */
void fs_close(int fd) {
    printf("关闭文件描述符: %d\n", fd);
    
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        printf("错误：无效的文件描述符: %d\n", fd);
        return;
    }
    
    if (g_fs_state.open_files[fd].reference_count == 0) {
        printf("错误：文件描述符 %d 未打开\n", fd);
        return;
    }
    
    // 减少引用计数
    g_fs_state.open_files[fd].reference_count--;
    
    // 如果引用计数为0，清空文件句柄
    if (g_fs_state.open_files[fd].reference_count == 0) {
        memset(&g_fs_state.open_files[fd], 0, sizeof(fs_file_handle_t));
        printf("文件描述符 %d 已关闭\n", fd);
    }
} 