/**
 * File System Operations Header
 * fs_ops.h
 * 
 * 提供文件系统核心操作的接口定义，包括格式化、挂载、同步等功能。
 * 
 * 设计理念：
 * - 提供清晰的文件系统操作接口
 * - 与磁盘模拟器紧密集成
 * - 支持完整的文件系统生命周期管理
 */

#ifndef _FS_OPS_H_
#define _FS_OPS_H_

#include "fs.h"
#include "disk_simulator.h"

/*==============================================================================
 * 文件系统操作常量
 *============================================================================*/

/* 文件系统布局常量 */
#define FS_SUPERBLOCK_BLOCK     0           // 超级块所在的块号
#define FS_INODE_BITMAP_BLOCK   1           // inode位图起始块号
#define FS_DATA_BITMAP_BLOCK    2           // 数据块位图起始块号
#define FS_INODE_TABLE_START    8           // inode表起始块号
#define FS_DATA_BLOCKS_START    128         // 数据块起始位置

/* 默认文件系统配置 */
#define FS_DEFAULT_MAX_INODES   1024        // 默认最大inode数量
#define FS_DEFAULT_MAX_BLOCKS   4096        // 默认最大数据块数量
#define FS_BITMAP_BLOCKS        4           // 位图占用的块数

/* 根目录配置 */
#define FS_ROOT_PERMISSIONS     0755        // 根目录权限 (rwxr-xr-x)
#define FS_ROOT_UID             0           // 根目录所有者UID
#define FS_ROOT_GID             0           // 根目录所有者GID

/*==============================================================================
 * 文件系统操作函数声明
 *============================================================================*/

/**
 * 格式化磁盘，创建新的文件系统
 * 
 * 该函数执行以下操作：
 * 1. 初始化超级块对象
 * 2. 将超级块写入磁盘的块0
 * 3. 初始化inode位图和数据块位图并写入磁盘
 * 4. 创建根目录('/')，初始化第一个inode和对应的数据块
 * 
 * @return FS_SUCCESS 成功，或相应的错误码
 */
void format_disk(void);

/**
 * 挂载文件系统
 * 
 * 从磁盘读取超级块和位图，初始化文件系统状态。
 * 
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_mount(void);

/**
 * 卸载文件系统
 * 
 * 同步所有修改到磁盘，清理内存状态。
 * 
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_unmount(void);

/**
 * 同步文件系统数据到磁盘
 * 
 * 将内存中的修改写入磁盘，确保数据持久性。
 * 
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_sync(void);

/**
 * 检查文件系统完整性
 * 
 * 验证文件系统结构的一致性，检测可能的数据损坏。
 * 
 * @return FS_SUCCESS 文件系统正常，或相应的错误码
 */
fs_error_t fs_ops_check(void);

/*==============================================================================
 * 辅助函数声明
 *============================================================================*/

/**
 * 初始化超级块
 * 
 * 创建并初始化超级块结构，设置文件系统元数据。
 * 
 * @param sb 指向超级块结构的指针
 * @param total_blocks 总块数
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_init_superblock(fs_superblock_t *sb, uint32_t total_blocks);

/**
 * 初始化位图
 * 
 * 创建并初始化inode位图和数据块位图。
 * 
 * @param bitmap 位图结构指针
 * @param total_bits 总位数
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_init_bitmap(fs_bitmap_t *bitmap, uint32_t total_bits);

/**
 * 创建根目录
 * 
 * 初始化根目录的inode和数据块，设置基本的目录项。
 * 
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_create_root_directory(void);

/**
 * 写入超级块到磁盘
 * 
 * 将超级块结构写入磁盘的第0块。
 * 
 * @param sb 超级块结构指针
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_write_superblock(const fs_superblock_t *sb);

/**
 * 从磁盘读取超级块
 * 
 * 从磁盘的第0块读取超级块结构。
 * 
 * @param sb 超级块结构指针
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_read_superblock(fs_superblock_t *sb);

/**
 * 写入位图到磁盘
 * 
 * 将位图数据写入指定的磁盘块。
 * 
 * @param bitmap 位图结构指针
 * @param start_block 起始块号
 * @param block_count 块数量
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_write_bitmap(const fs_bitmap_t *bitmap, uint32_t start_block, uint32_t block_count);

/**
 * 从磁盘读取位图
 * 
 * 从指定的磁盘块读取位图数据。
 * 
 * @param bitmap 位图结构指针
 * @param start_block 起始块号
 * @param block_count 块数量
 * @return FS_SUCCESS 成功，或相应的错误码
 */
fs_error_t fs_ops_read_bitmap(fs_bitmap_t *bitmap, uint32_t start_block, uint32_t block_count);

/*==============================================================================
 * 工具函数声明
 *============================================================================*/

/**
 * 计算校验和
 * 
 * 计算数据块的校验和，用于数据完整性验证。
 * 
 * @param data 数据指针
 * @param size 数据大小
 * @return 校验和值
 */
uint32_t fs_ops_calculate_checksum(const void *data, size_t size);

/**
 * 获取当前时间
 * 
 * 返回当前的Unix时间戳。
 * 
 * @return 当前时间戳
 */
time_t fs_ops_current_time(void);

/**
 * 打印文件系统状态
 * 
 * 显示文件系统的详细状态信息，用于调试和监控。
 */
void fs_ops_print_status(void);

/**
 * 错误码转换为字符串
 * 
 * 将文件系统错误码转换为可读的错误信息。
 * 
 * @param error 错误码
 * @return 错误信息字符串
 */
const char* fs_ops_error_to_string(fs_error_t error);

/*==============================================================================
 * 文件操作函数声明
 *============================================================================*/

/**
 * 创建文件
 * 
 * 查找空闲的inode，分配它，并在父目录中添加新的目录项。
 * 
 * @param path 文件路径
 * @return FS_SUCCESS 成功，或相应的错误码
 */
int fs_create(const char* path);

/**
 * 打开文件
 * 
 * 通过路径查找文件，执行权限检查（目前是占位符），并在全局打开文件表中分配文件描述符。
 * 
 * @param path 文件路径
 * @return 文件描述符索引（成功），或负数错误码（失败）
 */
int fs_open(const char* path);

/**
 * 关闭文件
 * 
 * 从打开文件表中释放文件描述符。
 * 
 * @param fd 文件描述符
 */
void fs_close(int fd);

/*==============================================================================
 * 用户管理系统需要的内部函数
 *============================================================================*/

/**
 * 读取指定inode
 */
fs_error_t fs_ops_read_inode(uint32_t inode_number, fs_inode_t *inode);

/**
 * 写入指定inode
 */
fs_error_t fs_ops_write_inode(uint32_t inode_number, const fs_inode_t *inode);

#endif /* _FS_OPS_H_ */ 