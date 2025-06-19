/**
 * File Operations Implementation
 * file_ops.c
 * 
 * 实现高级文件读写操作，包括fs_read和fs_write函数
 * 基于已有的文件系统、磁盘模拟器和格式化功能
 */

#include "file_ops.h"
#include "user_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*==============================================================================
 * 外部变量声明
 *============================================================================*/

// 从fs_ops.c引入全局文件系统状态
extern fs_state_t g_fs_state;

/*==============================================================================
 * 内部辅助函数声明
 *============================================================================*/

static fs_error_t load_filesystem_state_if_needed(void);
static fs_error_t read_inode_from_disk(uint32_t inode_number, fs_inode_t *inode);
static fs_error_t write_inode_to_disk(uint32_t inode_number, const fs_inode_t *inode);
static uint32_t alloc_data_block_from_bitmap(void);
static void free_data_block_to_bitmap(uint32_t block_num);
static time_t current_time(void);

/*==============================================================================
 * 文件读写操作实现
 *============================================================================*/

/**
 * 写入数据到文件
 */
int fs_write(int fd, const char* data, int size) {
    printf("写入文件: fd=%d, size=%d\n", fd, size);
    
    // 参数验证
    if (!data || size <= 0) {
        printf("错误：参数无效\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 验证文件描述符
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 确保文件系统状态已加载
    result = load_filesystem_state_if_needed();
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 获取文件句柄
    fs_file_handle_t *handle = &g_fs_state.open_files[fd];
    
    // 读取文件inode
    fs_inode_t inode;
    result = read_inode_from_disk(handle->inode_number, &inode);
    if (result != FS_SUCCESS) {
        printf("错误：读取inode失败\n");
        return result;
    }
    
    // 确保是普通文件
    if (inode.file_type != FS_FILE_TYPE_REGULAR) {
        printf("错误：不是普通文件\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 权限检查：检查当前用户是否有写权限
    if (!user_manager_check_permission(&inode, FS_PERM_OWNER_WRITE)) {
        printf("错误：权限不足 - 无法写入文件\n");
        return FS_ERROR_PERMISSION;
    }
    
    // 从当前文件位置开始写入
    uint64_t start_offset = handle->file_position;
    uint64_t end_offset = start_offset + size;
    int bytes_written = 0;
    
    printf("写入范围: %lu -> %lu\n", start_offset, end_offset);
    
    // 按块写入数据
    for (uint64_t current_offset = start_offset; current_offset < end_offset; ) {
        // 计算当前块位置
        uint32_t block_index, block_offset;
        file_ops_calculate_block_position(current_offset, &block_index, &block_offset);
        
        // 获取或分配数据块
        uint32_t block_num = file_ops_get_data_block(&inode, block_index);
        if (block_num == 0) {
            // 需要分配新块
            block_num = file_ops_allocate_data_block(&inode, block_index);
            if (block_num == 0) {
                printf("错误：无法分配数据块\n");
                break;
            }
            printf("分配新数据块: %u (索引: %u)\n", block_num, block_index);
        }
        
        // 读取现有块数据（用于部分写入）
        char block_data[BLOCK_SIZE];
        int disk_result = disk_read_block(block_num, block_data);
        if (disk_result != DISK_SUCCESS) {
            printf("错误：读取数据块失败\n");
            break;
        }
        
        // 计算本次写入的字节数
        uint32_t bytes_to_write = BLOCK_SIZE - block_offset;
        uint32_t remaining_bytes = size - bytes_written;
        if (bytes_to_write > remaining_bytes) {
            bytes_to_write = remaining_bytes;
        }
        
        // 复制数据到块中
        memcpy(block_data + block_offset, data + bytes_written, bytes_to_write);
        
        // 写入块到磁盘
        disk_result = disk_write_block(block_num, block_data);
        if (disk_result != DISK_SUCCESS) {
            printf("错误：写入数据块失败\n");
            break;
        }
        
        // 更新计数器
        bytes_written += bytes_to_write;
        current_offset += bytes_to_write;
        
        printf("写入块 %u: 偏移=%u, 字节=%u\n", block_num, block_offset, bytes_to_write);
    }
    
    // 更新inode信息
    if (bytes_written > 0) {
        // 更新文件大小
        uint64_t new_size = handle->file_position + bytes_written;
        if (new_size > inode.file_size) {
            inode.file_size = new_size;
            
            // 更新块计数（简化计算）
            inode.block_count = (inode.file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        }
        
        // 更新时间戳
        time_t now = current_time();
        inode.modify_time = now;
        inode.change_time = now;
        
        // 写回inode
        result = write_inode_to_disk(handle->inode_number, &inode);
        if (result != FS_SUCCESS) {
            printf("警告：更新inode失败\n");
        }
        
        // 更新文件位置
        handle->file_position += bytes_written;
        
        printf("文件写入完成: %d 字节，新位置: %lu，文件大小: %lu\n", 
               bytes_written, handle->file_position, inode.file_size);
    }
    
    return bytes_written;
}

/**
 * 从文件读取数据
 */
int fs_read(int fd, char* buffer, int size) {
    printf("读取文件: fd=%d, size=%d\n", fd, size);
    
    // 参数验证
    if (!buffer || size <= 0) {
        printf("错误：参数无效\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 验证文件描述符
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 确保文件系统状态已加载
    result = load_filesystem_state_if_needed();
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 获取文件句柄
    fs_file_handle_t *handle = &g_fs_state.open_files[fd];
    
    // 读取文件inode
    fs_inode_t inode;
    result = read_inode_from_disk(handle->inode_number, &inode);
    if (result != FS_SUCCESS) {
        printf("错误：读取inode失败\n");
        return result;
    }
    
    // 确保是普通文件
    if (inode.file_type != FS_FILE_TYPE_REGULAR) {
        printf("错误：不是普通文件\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 权限检查：检查当前用户是否有读权限
    if (!user_manager_check_permission(&inode, FS_PERM_OWNER_READ)) {
        printf("错误：权限不足 - 无法读取文件\n");
        return FS_ERROR_PERMISSION;
    }
    
    // 检查文件位置和大小
    if (handle->file_position >= inode.file_size) {
        printf("已到达文件末尾\n");
        return 0; // EOF
    }
    
    // 计算实际可读取的字节数
    uint64_t available_bytes = inode.file_size - handle->file_position;
    if ((uint64_t)size > available_bytes) {
        size = available_bytes;
    }
    
    uint64_t start_offset = handle->file_position;
    uint64_t end_offset = start_offset + size;
    int bytes_read = 0;
    
    printf("读取范围: %lu -> %lu (文件大小: %lu)\n", start_offset, end_offset, inode.file_size);
    
    // 按块读取数据
    for (uint64_t current_offset = start_offset; current_offset < end_offset; ) {
        // 计算当前块位置
        uint32_t block_index, block_offset;
        file_ops_calculate_block_position(current_offset, &block_index, &block_offset);
        
        // 获取数据块号
        uint32_t block_num = file_ops_get_data_block(&inode, block_index);
        if (block_num == 0) {
            printf("错误：数据块未分配 (索引: %u)\n", block_index);
            break;
        }
        
        // 读取块数据
        char block_data[BLOCK_SIZE];
        int disk_result = disk_read_block(block_num, block_data);
        if (disk_result != DISK_SUCCESS) {
            printf("错误：读取数据块失败\n");
            break;
        }
        
        // 计算本次读取的字节数
        uint32_t bytes_to_read = BLOCK_SIZE - block_offset;
        uint32_t remaining_bytes = size - bytes_read;
        if (bytes_to_read > remaining_bytes) {
            bytes_to_read = remaining_bytes;
        }
        
        // 复制数据到缓冲区
        memcpy(buffer + bytes_read, block_data + block_offset, bytes_to_read);
        
        // 更新计数器
        bytes_read += bytes_to_read;
        current_offset += bytes_to_read;
        
        printf("读取块 %u: 偏移=%u, 字节=%u\n", block_num, block_offset, bytes_to_read);
    }
    
    // 更新文件位置和访问时间
    if (bytes_read > 0) {
        handle->file_position += bytes_read;
        
        // 更新访问时间
        inode.access_time = current_time();
        write_inode_to_disk(handle->inode_number, &inode);
        
        printf("文件读取完成: %d 字节，新位置: %lu\n", bytes_read, handle->file_position);
    }
    
    return bytes_read;
}

/**
 * 设置文件位置指针
 */
int fs_seek(int fd, int offset, int whence) {
    printf("文件定位: fd=%d, offset=%d, whence=%d\n", fd, offset, whence);
    
    // 验证文件描述符
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 确保文件系统状态已加载
    result = load_filesystem_state_if_needed();
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 获取文件句柄
    fs_file_handle_t *handle = &g_fs_state.open_files[fd];
    
    // 读取文件inode获取文件大小
    fs_inode_t inode;
    result = read_inode_from_disk(handle->inode_number, &inode);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 计算新位置
    uint64_t new_position;
    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
            new_position = handle->file_position + offset;
            break;
        case SEEK_END:
            new_position = inode.file_size + offset;
            break;
        default:
            printf("错误：无效的whence参数\n");
            return FS_ERROR_INVALID_PARAM;
    }
    
    // 验证新位置（允许超过文件末尾，用于写入扩展文件）
    if ((int64_t)new_position < 0) {
        printf("错误：文件位置不能为负数\n");
        return FS_ERROR_INVALID_PARAM;
    }
    
    handle->file_position = new_position;
    printf("文件位置已设置为: %lu\n", new_position);
    
    return new_position;
}

/**
 * 获取当前文件位置
 */
int fs_tell(int fd) {
    // 验证文件描述符
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    return g_fs_state.open_files[fd].file_position;
}

/**
 * 获取文件大小
 */
int fs_size(int fd) {
    // 验证文件描述符
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 确保文件系统状态已加载
    result = load_filesystem_state_if_needed();
    if (result != FS_SUCCESS) {
        return result;
    }
    
    // 获取文件句柄
    fs_file_handle_t *handle = &g_fs_state.open_files[fd];
    
    // 读取文件inode
    fs_inode_t inode;
    result = read_inode_from_disk(handle->inode_number, &inode);
    if (result != FS_SUCCESS) {
        return result;
    }
    
    return inode.file_size;
}

/*==============================================================================
 * 辅助函数实现
 *============================================================================*/

/**
 * 计算文件偏移量对应的块号和块内偏移
 */
void file_ops_calculate_block_position(uint64_t file_offset, uint32_t* block_index, uint32_t* block_offset) {
    if (block_index) {
        *block_index = file_offset / BLOCK_SIZE;
    }
    if (block_offset) {
        *block_offset = file_offset % BLOCK_SIZE;
    }
}

/**
 * 获取inode中指定索引的数据块号
 */
uint32_t file_ops_get_data_block(const fs_inode_t* inode, uint32_t block_index) {
    if (!inode) {
        return 0;
    }
    
    // 只支持直接块（简化实现）
    if (block_index < DIRECT_BLOCKS) {
        return inode->direct_blocks[block_index];
    }
    
    // TODO: 支持间接块
    printf("警告：暂不支持间接块 (索引: %u)\n", block_index);
    return 0;
}

/**
 * 分配并设置inode中指定索引的数据块
 */
uint32_t file_ops_allocate_data_block(fs_inode_t* inode, uint32_t block_index) {
    if (!inode) {
        return 0;
    }
    
    // 只支持直接块（简化实现）
    if (block_index < DIRECT_BLOCKS) {
        if (inode->direct_blocks[block_index] == 0) {
            uint32_t new_block = alloc_data_block_from_bitmap();
            if (new_block > 0) {
                inode->direct_blocks[block_index] = new_block;
                return new_block;
            }
        }
        return inode->direct_blocks[block_index];
    }
    
    // TODO: 支持间接块
    printf("警告：暂不支持间接块分配 (索引: %u)\n", block_index);
    return 0;
}

/**
 * 验证文件描述符的有效性
 */
fs_error_t file_ops_validate_fd(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        printf("错误：文件描述符超出范围: %d\n", fd);
        return FS_ERROR_INVALID_PARAM;
    }
    
    if (g_fs_state.open_files[fd].reference_count == 0) {
        printf("错误：文件描述符未打开: %d\n", fd);
        return FS_ERROR_INVALID_PARAM;
    }
    
    return FS_SUCCESS;
}

/*==============================================================================
 * 内部辅助函数实现
 *============================================================================*/

/**
 * 如果需要，加载文件系统状态
 */
static fs_error_t load_filesystem_state_if_needed(void) {
    if (g_fs_state.superblock.magic_number != FS_MAGIC_NUMBER) {
        printf("加载文件系统状态...\n");
        
        // 读取超级块
        fs_error_t result = fs_ops_read_superblock(&g_fs_state.superblock);
        if (result != FS_SUCCESS) {
            return result;
        }
        
        // 读取位图
        result = fs_ops_read_bitmap(&g_fs_state.inode_bitmap, FS_INODE_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
        if (result != FS_SUCCESS) {
            return result;
        }
        
        result = fs_ops_read_bitmap(&g_fs_state.block_bitmap, FS_DATA_BITMAP_BLOCK, FS_BITMAP_BLOCKS);
        if (result != FS_SUCCESS) {
            return result;
        }
        
        printf("文件系统状态加载完成\n");
    }
    
    return FS_SUCCESS;
}

/**
 * 从磁盘读取inode
 */
static fs_error_t read_inode_from_disk(uint32_t inode_number, fs_inode_t *inode) {
    if (!inode || inode_number == 0 || inode_number >= g_fs_state.superblock.total_inodes) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 计算inode在inode表中的位置
    uint32_t inodes_per_block = BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = g_fs_state.superblock.inode_table_start + (inode_number / inodes_per_block);
    uint32_t inode_offset = (inode_number % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取inode块
    char inode_block[BLOCK_SIZE];
    int result = disk_read_block(inode_block_num, inode_block);
    if (result != DISK_SUCCESS) {
        return FS_ERROR_IO;
    }
    
    // 复制inode数据
    memcpy(inode, inode_block + inode_offset, sizeof(fs_inode_t));
    
    return FS_SUCCESS;
}

/**
 * 将inode写入磁盘
 */
static fs_error_t write_inode_to_disk(uint32_t inode_number, const fs_inode_t *inode) {
    if (!inode || inode_number == 0 || inode_number >= g_fs_state.superblock.total_inodes) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 计算inode在inode表中的位置
    uint32_t inodes_per_block = BLOCK_SIZE / sizeof(fs_inode_t);
    uint32_t inode_block_num = g_fs_state.superblock.inode_table_start + (inode_number / inodes_per_block);
    uint32_t inode_offset = (inode_number % inodes_per_block) * sizeof(fs_inode_t);
    
    // 读取inode块
    char inode_block[BLOCK_SIZE];
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
 * 从位图分配数据块
 */
static uint32_t alloc_data_block_from_bitmap(void) {
    if (g_fs_state.block_bitmap.free_count == 0) {
        return 0; // 没有可用的块
    }
    
    // 从位图分配一个位
    for (uint32_t i = 0; i < g_fs_state.block_bitmap.total_bits; i++) {
        uint32_t bit_num = (g_fs_state.block_bitmap.last_allocated + i) % g_fs_state.block_bitmap.total_bits;
        uint32_t byte_index = bit_num / 8;
        uint32_t bit_offset = bit_num % 8;
        
        // 检查这个位是否空闲
        if (!(g_fs_state.block_bitmap.bitmap[byte_index] & (1 << bit_offset))) {
            // 设置这个位为已使用
            g_fs_state.block_bitmap.bitmap[byte_index] |= (1 << bit_offset);
            g_fs_state.block_bitmap.free_count--;
            g_fs_state.block_bitmap.last_allocated = bit_num;
            
            // 转换为绝对块号
            uint32_t block_num = bit_num + g_fs_state.superblock.data_blocks_start;
            return block_num;
        }
    }
    
    return 0; // 没有找到空闲的块
}

/**
 * 释放数据块到位图
 */
static void free_data_block_to_bitmap(uint32_t block_num) {
    if (block_num < g_fs_state.superblock.data_blocks_start) {
        return; // 不是数据块
    }
    
    uint32_t bit_num = block_num - g_fs_state.superblock.data_blocks_start;
    if (bit_num >= g_fs_state.block_bitmap.total_bits) {
        return; // 超出范围
    }
    
    uint32_t byte_index = bit_num / 8;
    uint32_t bit_offset = bit_num % 8;
    
    // 只有当位当前是已分配状态时才释放
    if (g_fs_state.block_bitmap.bitmap[byte_index] & (1 << bit_offset)) {
        g_fs_state.block_bitmap.bitmap[byte_index] &= ~(1 << bit_offset);
        g_fs_state.block_bitmap.free_count++;
    }
}

/**
 * 获取当前时间
 */
static time_t current_time(void) {
    return time(NULL);
} 