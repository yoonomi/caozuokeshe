# 模拟文件系统设计与实现

## 项目概述

本项目实现了一个简化的模拟文件系统，包含完整的用户管理、文件操作、目录管理等功能。该文件系统采用类似UNIX的设计理念，支持多用户、权限控制、inode管理等特性。

## 设计特点

### 核心架构
- **分层设计**: 用户管理层、文件系统层、inode层、数据块层
- **模块化实现**: 每个功能模块独立实现，接口清晰
- **类UNIX设计**: 借鉴UNIX文件系统的设计思想
- **内存文件系统**: 数据存储在内存中，便于测试和演示

### 主要功能
1. **用户管理系统**
   - 多用户支持
   - 用户认证
   - 权限控制
   - 用户组管理

2. **文件操作**
   - 文件创建、删除
   - 文件读写
   - 文件权限管理
   - 文件元数据管理

3. **目录系统**
   - 目录创建、删除
   - 目录浏览
   - 路径解析

4. **存储管理**
   - inode分配和回收
   - 数据块管理
   - 空间统计

## 文件结构

```
Project_C_FileSystem/
├── fs.h                    # 核心头文件，定义数据结构
├── main.c                  # 程序入口，模拟文件系统交互
├── user.c                  # 用户管理模块
├── inode.c                 # inode管理模块
├── file_ops.c              # 文件操作模块
├── fs_ops.c                # 文件系统操作模块
├── Makefile                # 编译脚本
└── README.md               # 本说明文档
```

## 数据结构设计

### 核心数据结构

#### 1. 超级块 (superblock_t)
```c
typedef struct {
    uint32_t magic_number;      // 魔数
    uint32_t total_blocks;      // 总块数
    uint32_t free_blocks;       // 空闲块数
    uint32_t total_inodes;      // 总inode数
    uint32_t free_inodes;       // 空闲inode数
    uint32_t root_inode;        // 根目录inode
    time_t created_time;        // 创建时间
    time_t last_mount_time;     // 最后挂载时间
    uint32_t mount_count;       // 挂载次数
} superblock_t;
```

#### 2. inode结构 (inode_t)
```c
typedef struct {
    uint32_t inode_id;          // inode编号
    file_type_t type;           // 文件类型
    uint32_t size;              // 文件大小
    uint32_t owner_uid;         // 所有者用户ID
    uint32_t owner_gid;         // 所有者组ID
    uint16_t permissions;       // 权限位
    time_t created_time;        // 创建时间
    time_t modified_time;       // 修改时间
    time_t accessed_time;       // 访问时间
    uint32_t link_count;        // 链接计数
    uint32_t block_count;       // 占用块数
    uint32_t direct_blocks[12]; // 直接块索引
    uint32_t indirect_block;    // 间接块索引
    int is_used;                // 是否被使用
} inode_t;
```

#### 3. 用户结构 (user_t)
```c
typedef struct {
    uint32_t uid;               // 用户ID
    char username[64];          // 用户名
    char password[64];          // 密码
    uint32_t gid;               // 组ID
    time_t created_time;        // 创建时间
    int is_active;              // 是否活跃
} user_t;
```

## 编译和运行

### 系统要求
- Linux操作系统
- GCC编译器
- Make工具

### 编译步骤
```bash
# 编译项目
make

# 或者编译并运行
make run

# 清理编译文件
make clean
```

### 运行程序
```bash
./filesystem
```

## 使用指南

### 基本操作流程

#### 1. 启动系统
```bash
./filesystem
```
系统将自动初始化并挂载文件系统。

#### 2. 用户管理
```bash
# 创建用户
adduser alice password123 100

# 用户登录
login alice password123

# 查看当前用户
whoami

# 用户登出
logout
```

#### 3. 文件操作
```bash
# 创建文件
create myfile.txt

# 打开文件
open myfile.txt

# 写入文件 (假设文件ID为0)
write 0 Hello, World!

# 读取文件
read 0 20

# 关闭文件
close 0

# 删除文件
delete myfile.txt
```

#### 4. 目录操作
```bash
# 创建目录
mkdir mydir

# 列出目录内容
ls

# 查看文件信息
stat myfile.txt
```

#### 5. 系统管理
```bash
# 查看系统状态
status

# 显示帮助
help

# 退出系统
quit
```

### 高级功能

#### 权限管理
- 文件权限采用UNIX风格的8进制表示 (如 755, 644)
- 支持所有者、组、其他用户的读写执行权限
- root用户拥有所有权限

#### 文件类型
- 普通文件: 存储数据的常规文件
- 目录: 包含其他文件和目录的容器

## 系统配置

### 可配置参数 (fs.h)
```c
#define MAX_FILENAME_LEN    64      // 最大文件名长度
#define MAX_PATH_LEN        256     // 最大路径长度
#define BLOCK_SIZE          1024    // 块大小
#define MAX_USERS           32      // 最大用户数
#define MAX_FILES           1024    // 最大文件数
#define MAX_INODES          1024    // 最大inode数
#define MAX_FILE_SIZE       (1024*1024)  // 最大文件大小 1MB
#define MAX_OPEN_FILES      64      // 最大同时打开文件数
```

## 测试用例

### 基本功能测试
```bash
# 1. 用户管理测试
adduser test test123 100
login test test123
whoami
logout

# 2. 文件操作测试
login root root123
create test.txt
open test.txt
write 0 "This is a test file"
read 0 50
close 0
stat test.txt

# 3. 目录操作测试
mkdir testdir
ls
delete test.txt
ls
```

### 权限测试
```bash
# 创建用户和文件
adduser user1 pass1 100
adduser user2 pass2 100
login user1 pass1
create private.txt

# 测试权限控制
logout
login user2 pass2
open private.txt  # 应该失败
```

## 性能特性

### 空间复杂度
- inode表: O(MAX_INODES)
- 用户表: O(MAX_USERS)
- 数据块: O(MAX_FILES)

### 时间复杂度
- 文件查找: O(n) - n为目录中文件数
- inode分配: O(MAX_INODES)
- 用户查找: O(MAX_USERS)

## 局限性和改进方向

### 当前局限性
1. **单级目录**: 目前只支持扁平目录结构
2. **内存存储**: 数据不持久化，重启后丢失
3. **简化实现**: 许多功能为简化版本
4. **性能优化**: 查找算法可以优化

### 改进方向
1. **多级目录**: 实现完整的目录树结构
2. **磁盘持久化**: 添加数据持久化功能
3. **索引优化**: 使用B+树或哈希表优化查找
4. **并发控制**: 添加文件锁和并发访问控制
5. **缓存机制**: 实现inode和数据块缓存
6. **碎片整理**: 添加磁盘空间整理功能

## 错误码说明

```c
typedef enum {
    FS_SUCCESS = 0,         // 成功
    FS_ERROR_INVALID_PARAM, // 无效参数
    FS_ERROR_FILE_NOT_FOUND,// 文件未找到
    FS_ERROR_FILE_EXISTS,   // 文件已存在
    FS_ERROR_NO_SPACE,      // 空间不足
    FS_ERROR_PERMISSION,    // 权限错误
    FS_ERROR_NOT_DIRECTORY, // 不是目录
    FS_ERROR_IS_DIRECTORY,  // 是目录
    FS_ERROR_FILE_OPEN,     // 文件已打开
    FS_ERROR_USER_EXISTS,   // 用户已存在
    FS_ERROR_USER_NOT_FOUND,// 用户未找到
    FS_ERROR_INVALID_USER,  // 无效用户
    FS_ERROR_MAX_FILES,     // 达到最大文件数
    FS_ERROR_IO             // IO错误
} fs_error_t;
```

## 开发团队

本项目作为操作系统课程设计的一部分，展示了文件系统的基本原理和实现方法。

## 许可证

本项目仅供学习和教育目的使用。 