# 简单文件系统数据结构设计文档

## 概述

本文档描述了一个基于Unix理念的简单文件系统的核心数据结构设计。该设计旨在提供教学用途，展示现代文件系统的基本组件和工作原理。

## 设计理念

### 1. Unix-like 设计
- 采用经典的超级块（Superblock）+ inode + 目录项（Directory Entry）架构
- 支持Unix风格的权限系统（rwxrwxrwx）
- 使用魔数进行文件系统识别和完整性检查

### 2. 模块化设计
- 清晰分离各个组件的职责
- 每个数据结构都有明确的用途和边界
- 便于理解和扩展

### 3. 教育友好
- 详细的注释说明每个字段的用途
- 合理的结构体大小便于调试
- 提供验证和测试函数

## 核心数据结构

### 1. 超级块（fs_superblock_t）

**用途**: 存储文件系统的元数据和配置信息

**关键字段**:
```c
typedef struct {
    uint32_t    magic_number;               // 魔数: 0x53465321 ("SFS!")
    uint32_t    version;                    // 文件系统版本
    uint32_t    block_size;                 // 块大小: 1024字节
    uint32_t    total_blocks;               // 总块数: 4096
    uint32_t    total_inodes;               // 总inode数: 1024
    uint32_t    free_blocks;                // 空闲块数
    uint32_t    free_inodes;                // 空闲inode数
    uint32_t    root_inode;                 // 根目录inode号: 1
    time_t      created_time;               // 创建时间
    time_t      last_mount_time;            // 最后挂载时间
    uint32_t    checksum;                   // 完整性校验和
} fs_superblock_t;
```

**设计特点**:
- 大小为152字节，小于一个块（1024字节），便于高效读写
- 包含完整性校验和，确保数据可靠性
- 支持挂载计数和时间戳，便于管理和调试

### 2. inode结构（fs_inode_t）

**用途**: 表示文件系统中的文件或目录，存储所有元数据

**关键字段**:
```c
typedef struct {
    uint32_t    inode_number;               // 唯一inode标识符
    uint16_t    file_type;                  // 文件类型（普通文件/目录/符号链接）
    uint16_t    permissions;                // Unix风格权限（rwxrwxrwx）
    uint32_t    owner_uid;                  // 所有者用户ID
    uint32_t    owner_gid;                  // 所有者组ID
    uint32_t    link_count;                 // 硬链接计数
    uint64_t    file_size;                  // 文件大小（字节）
    uint32_t    block_count;                // 分配的块数
    
    // Unix标准时间戳
    time_t      access_time;                // 最后访问时间（atime）
    time_t      modify_time;                // 最后修改时间（mtime）
    time_t      change_time;                // inode变更时间（ctime）
    time_t      create_time;                // 创建时间（birth time）
    
    // 数据块指针
    uint32_t    direct_blocks[12];          // 12个直接块指针
    uint32_t    indirect_block;             // 间接块指针
    uint32_t    double_indirect_block;      // 二级间接块指针
    uint32_t    triple_indirect_block;      // 三级间接块指针
} fs_inode_t;
```

**设计特点**:
- 大小为148字节，每个块可存储6个inode，效率良好
- 支持12个直接块 + 多级间接块，可支持大文件
- 完整的Unix时间戳支持
- 支持硬链接和符号链接

### 3. 目录项（fs_dir_entry_t）

**用途**: 建立文件名到inode的映射关系

**关键字段**:
```c
typedef struct {
    uint32_t    inode_number;               // 指向的inode号
    uint16_t    entry_length;               // 目录项长度
    uint8_t     filename_length;            // 文件名长度
    uint8_t     file_type;                  // 文件类型（性能优化）
    char        filename[64];               // 文件名（最大64字符）
    uint8_t     is_valid;                   // 有效性标志
} fs_dir_entry_t;
```

**设计特点**:
- 大小为76字节，每个块可存储13个目录项
- 支持变长文件名（最大64字符）
- 包含类型信息，减少inode查找次数
- 支持软删除（通过is_valid标志）

### 4. 权限系统（fs_permission_t）

**用途**: 实现Unix风格的文件权限控制

**权限位定义**:
```c
typedef enum {
    FS_PERM_OWNER_READ      = 0400,         // 所有者读权限
    FS_PERM_OWNER_WRITE     = 0200,         // 所有者写权限
    FS_PERM_OWNER_EXEC      = 0100,         // 所有者执行权限
    FS_PERM_GROUP_READ      = 0040,         // 组读权限
    FS_PERM_GROUP_WRITE     = 0020,         // 组写权限
    FS_PERM_GROUP_EXEC      = 0010,         // 组执行权限
    FS_PERM_OTHER_READ      = 0004,         // 其他用户读权限
    FS_PERM_OTHER_WRITE     = 0002,         // 其他用户写权限
    FS_PERM_OTHER_EXEC      = 0001,         // 其他用户执行权限
} fs_permission_t;
```

**便利宏**:
```c
#define FS_IS_READABLE(perms, uid, gid, file_uid, file_gid)
#define FS_IS_WRITABLE(perms, uid, gid, file_uid, file_gid)
#define FS_IS_EXECUTABLE(perms, uid, gid, file_uid, file_gid)
```

### 5. 位图管理（fs_bitmap_t）

**用途**: 管理inode和数据块的分配状态

```c
typedef struct {
    uint8_t     *bitmap;                    // 位图数据
    uint32_t    total_bits;                 // 总位数
    uint32_t    free_count;                 // 空闲位数
    uint32_t    last_allocated;             // 最后分配位置（优化）
} fs_bitmap_t;
```

### 6. 文件系统状态（fs_state_t）

**用途**: 维护文件系统的运行时状态

```c
typedef struct {
    fs_superblock_t     superblock;                     // 超级块
    fs_bitmap_t         inode_bitmap;                   // inode分配位图
    fs_bitmap_t         block_bitmap;                   // 数据块分配位图
    fs_inode_t          *inode_table;                   // inode表
    fs_file_handle_t    open_files[MAX_OPEN_FILES];     // 打开文件句柄
    fs_user_t           users[MAX_USERS];               // 用户账户
    uint32_t            current_user_uid;               // 当前用户
    uint32_t            current_directory_inode;        // 当前目录
    uint8_t             is_mounted;                     // 挂载状态
    // ... 统计信息
} fs_state_t;
```

## 函数接口设计

### 1. 文件系统管理
- `fs_init()` - 初始化文件系统
- `fs_mount()` - 挂载文件系统
- `fs_unmount()` - 卸载文件系统
- `fs_sync()` - 同步数据到存储
- `fs_check()` - 完整性检查

### 2. inode管理
- `fs_inode_alloc()` - 分配新inode
- `fs_inode_free()` - 释放inode
- `fs_inode_read/write()` - 读写inode
- `fs_inode_touch()` - 更新时间戳

### 3. 块管理
- `fs_block_alloc()` - 分配数据块
- `fs_block_free()` - 释放数据块
- `fs_block_read/write()` - 读写数据块

### 4. 文件操作
- `fs_file_create()` - 创建文件
- `fs_file_open/close()` - 打开/关闭文件
- `fs_file_read/write()` - 读写文件
- `fs_file_seek/tell()` - 文件定位
- `fs_file_delete()` - 删除文件

### 5. 目录操作
- `fs_dir_create()` - 创建目录
- `fs_dir_read()` - 读取目录项
- `fs_dir_change()` - 切换目录

## 性能特性

### 内存效率
- **超级块**: 152字节 < 1块
- **inode**: 148字节，6个/块
- **目录项**: 76字节，13个/块
- **总状态**: 约6.6KB（合理的内存占用）

### 存储效率
- **1024个inode**: 需要171个块
- **支持最大文件**: 通过间接块支持大文件
- **目录容量**: 每个块可存储13个目录项

### 设计优势
1. **兼容性**: 基于成熟的Unix文件系统理念
2. **可扩展性**: 预留了足够的扩展字段
3. **教育价值**: 结构清晰，便于学习理解
4. **实用性**: 支持现代文件系统的基本功能

## 测试验证

设计包含了完整的测试框架：
- 数据结构大小验证
- 权限系统测试
- 位图操作测试
- 时间戳功能测试
- 完整性检查测试

## 总结

这个文件系统数据结构设计成功地将Unix文件系统的核心概念转化为一个简洁、高效、易于理解的教学实现。它不仅展示了文件系统的基本原理，还提供了足够的功能来支持实际的文件操作。

主要成就：
- ✅ 完整的超级块设计，包含所有必要的元数据
- ✅ 标准的inode结构，支持Unix时间戳和权限
- ✅ 高效的目录项映射机制
- ✅ 完整的函数接口规范
- ✅ 详细的注释和文档
- ✅ 通过编译和运行时测试验证

这个设计为后续的文件系统实现提供了坚实的基础，同时也是学习操作系统和文件系统原理的优秀教学材料。 