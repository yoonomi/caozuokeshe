/**
 * File Operations Header
 * file_ops.h
 * 
 * 文件读写操作的接口定义
 * 提供高级文件I/O操作，基于已有的文件系统和磁盘模拟器
 */

#ifndef _FILE_OPS_H_
#define _FILE_OPS_H_

#include "fs.h"
#include "fs_ops.h"
#include "disk_simulator.h"
#include <stdint.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/

// 文件操作标志位
#define FILE_OP_READ        0x01        // 读操作
#define FILE_OP_WRITE       0x02        // 写操作
#define FILE_OP_APPEND      0x04        // 追加写操作

// 搜索模式定义（用于文件定位）
#define SEEK_SET            0           // 从文件开头
#define SEEK_CUR            1           // 从当前位置
#define SEEK_END            2           // 从文件末尾

/*==============================================================================
 * 文件读写操作函数声明
 *============================================================================*/

/**
 * 写入数据到文件
 * 
 * 基于文件描述符，找到inode。根据文件当前偏移量确定要写入的数据块。
 * 如果需要新块，从数据块位图中分配它们。将数据写入模拟磁盘并更新inode的大小。
 * 
 * @param fd 文件描述符
 * @param data 要写入的数据缓冲区
 * @param size 要写入的字节数
 * @return 实际写入的字节数（成功），或负数错误码（失败）
 */
int fs_write(int fd, const char* data, int size);

/**
 * 从文件读取数据
 * 
 * 从文件的数据块中读取数据到提供的缓冲区中。
 * 
 * @param fd 文件描述符
 * @param buffer 存储读取数据的缓冲区
 * @param size 要读取的最大字节数
 * @return 实际读取的字节数（成功），或负数错误码（失败）
 */
int fs_read(int fd, char* buffer, int size);

/**
 * 设置文件位置指针
 * 
 * 移动文件读写位置到指定偏移量。
 * 
 * @param fd 文件描述符
 * @param offset 偏移量
 * @param whence 搜索模式 (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return 新的文件位置（成功），或负数错误码（失败）
 */
int fs_seek(int fd, int offset, int whence);

/**
 * 获取当前文件位置
 * 
 * 返回当前文件读写位置。
 * 
 * @param fd 文件描述符
 * @return 当前文件位置（成功），或负数错误码（失败）
 */
int fs_tell(int fd);

/**
 * 获取文件大小
 * 
 * 返回文件的当前大小（字节数）。
 * 
 * @param fd 文件描述符
 * @return 文件大小（成功），或负数错误码（失败）
 */
int fs_size(int fd);

/*==============================================================================
 * 辅助函数声明（内部使用）
 *============================================================================*/

/**
 * 计算文件偏移量对应的块号和块内偏移
 * 
 * @param file_offset 文件偏移量
 * @param block_index 输出：块索引
 * @param block_offset 输出：块内偏移
 */
void file_ops_calculate_block_position(uint64_t file_offset, uint32_t* block_index, uint32_t* block_offset);

/**
 * 获取inode中指定索引的数据块号
 * 
 * @param inode inode结构指针
 * @param block_index 块索引
 * @return 数据块号，0表示未分配
 */
uint32_t file_ops_get_data_block(const fs_inode_t* inode, uint32_t block_index);

/**
 * 分配并设置inode中指定索引的数据块
 * 
 * @param inode inode结构指针
 * @param block_index 块索引
 * @return 分配的数据块号，0表示失败
 */
uint32_t file_ops_allocate_data_block(fs_inode_t* inode, uint32_t block_index);

/**
 * 验证文件描述符的有效性
 * 
 * @param fd 文件描述符
 * @return FS_SUCCESS或相应错误码
 */
fs_error_t file_ops_validate_fd(int fd);

#endif /* _FILE_OPS_H_ */ 