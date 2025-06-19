# 磁盘模拟器

这是一个使用单个主机OS文件模拟基于块的磁盘存储的完整实现。磁盘模拟器提供了可靠的块I/O操作，具有全面的错误处理和性能监控功能。

## 文件组成

- `disk_simulator.h` - 磁盘模拟器头文件
- `disk_simulator.c` - 磁盘模拟器实现
- `disk_test.c` - 测试程序
- `disk_demo.c` - 演示程序

## 核心功能

### 基本操作

1. **`disk_init(const char* filename, int disk_size)`**
   - 创建或打开代表磁盘的文件
   - 如果文件不存在，将创建指定大小的新磁盘
   - 如果文件存在，将验证并打开现有磁盘

2. **`disk_write_block(int block_num, const char* data)`**
   - 向指定块号写入一个块的数据
   - 块号从0开始，数据大小为1024字节

3. **`disk_read_block(int block_num, char* buffer)`**
   - 从指定块号读取一个块的数据
   - 数据读入提供的缓冲区

### 扩展功能

- **多块操作**: `disk_write_blocks()`, `disk_read_blocks()`
- **工具函数**: `disk_zero_block()`, `disk_copy_block()`
- **磁盘管理**: `disk_sync()`, `disk_close()`, `disk_format()`
- **信息查询**: `disk_get_info()`, `disk_get_stats()`
- **状态监控**: `disk_print_status()`, `disk_is_initialized()`

## 设计特性

### 磁盘格式

- **魔数验证**: 使用"DSK!"魔数确保文件格式正确
- **版本控制**: 支持版本检查和升级
- **校验和保护**: 头部数据包含校验和防止损坏
- **时间戳记录**: 记录创建和访问时间

### 错误处理

```c
typedef enum {
    DISK_SUCCESS            = 0,    // 操作成功
    DISK_ERROR_INVALID_PARAM = -1,  // 无效参数
    DISK_ERROR_FILE_OPEN    = -2,   // 文件打开失败
    DISK_ERROR_FILE_CREATE  = -3,   // 文件创建失败
    DISK_ERROR_FILE_READ    = -4,   // 文件读取失败
    DISK_ERROR_FILE_WRITE   = -5,   // 文件写入失败
    DISK_ERROR_FILE_SEEK    = -6,   // 文件定位失败
    DISK_ERROR_BLOCK_RANGE  = -7,   // 块号超出范围
    DISK_ERROR_NOT_INIT     = -8,   // 磁盘未初始化
    DISK_ERROR_ALREADY_INIT = -9,   // 磁盘已初始化
    DISK_ERROR_DISK_FULL    = -10,  // 磁盘已满
    DISK_ERROR_IO           = -11,  // I/O错误
    DISK_ERROR_CORRUPTED    = -12   // 磁盘数据损坏
} disk_error_t;
```

### 性能监控

磁盘模拟器提供详细的统计信息：

- 读写操作次数
- 传输字节数
- 错误计数
- 平均操作时间
- 最后操作时间

## 编译说明

### 基本编译

```bash
gcc -Wall -Wextra -std=c99 -o disk_test disk_simulator.c disk_test.c -lrt
gcc -Wall -Wextra -std=c99 -o disk_demo disk_simulator.c disk_demo.c -lrt
```

### 编译选项说明

- `-std=c99`: 使用C99标准
- `-lrt`: 链接实时库（用于高精度时间函数）
- `-Wall -Wextra`: 启用额外的警告

## 使用示例

### 基本用法

```c
#include "disk_simulator.h"

int main() {
    // 初始化磁盘
    int result = disk_init("my_disk.img", 1024 * 1024);  // 1MB磁盘
    if (result != DISK_SUCCESS) {
        printf("磁盘初始化失败: %s\n", disk_error_to_string(result));
        return -1;
    }
    
    // 写入数据
    char data[DISK_BLOCK_SIZE];
    memset(data, 0xAA, DISK_BLOCK_SIZE);
    result = disk_write_block(0, data);
    
    // 读取数据
    char buffer[DISK_BLOCK_SIZE];
    result = disk_read_block(0, buffer);
    
    // 清理
    disk_close();
    return 0;
}
```

### 高级用法

```c
// 多块操作
char multi_data[5 * DISK_BLOCK_SIZE];
disk_write_blocks(10, 5, multi_data);  // 写入5个连续块

// 工具函数
disk_zero_block(20);          // 清零块20
disk_copy_block(0, 21);       // 复制块0到块21

// 格式化磁盘
disk_format(0x00);           // 用0填充整个磁盘

// 获取统计信息
disk_stats_t stats;
disk_get_stats(&stats);
printf("总读取次数: %lu\n", stats.total_reads);
```

## 运行测试

### 功能测试

```bash
./disk_test
```

测试程序包含以下测试用例：

1. 磁盘初始化测试
2. 读写操作测试
3. 磁盘格式化测试
4. 多块操作测试
5. 工具函数测试
6. 统计功能测试
7. 磁盘持久性测试

### 功能演示

```bash
./disk_demo
```

演示程序展示所有主要功能的使用方法。

## 技术规格

### 存储规格

- **块大小**: 1024字节
- **最大磁盘大小**: 受主机文件系统限制
- **块地址**: 32位无符号整数（支持4TB磁盘）
- **文件头部**: 包含完整的磁盘元数据

### 性能特性

- **零拷贝设计**: 直接使用系统调用进行I/O
- **高精度计时**: 使用POSIX实时时钟
- **缓存友好**: 块对齐的数据结构
- **错误恢复**: 完整的错误处理和状态恢复

### 兼容性

- **操作系统**: Linux, Unix系统
- **编译器**: GCC, Clang
- **C标准**: C99或更新版本
- **依赖库**: POSIX实时库（librt）

## 与文件系统集成

磁盘模拟器设计为文件系统的底层存储抽象：

```c
// 文件系统可以这样使用磁盘模拟器
int fs_read_superblock(fs_superblock_t* sb) {
    char buffer[DISK_BLOCK_SIZE];
    int result = disk_read_block(0, buffer);  // 读取超级块
    if (result == DISK_SUCCESS) {
        memcpy(sb, buffer, sizeof(fs_superblock_t));
    }
    return result;
}
```

## 设计优势

1. **模块化设计**: 独立的磁盘抽象层
2. **易于测试**: 完整的测试覆盖
3. **性能监控**: 内置统计和诊断
4. **错误处理**: 全面的错误码系统
5. **数据完整性**: 校验和和验证机制
6. **教学友好**: 清晰的接口和文档

## 故障排除

### 常见问题

1. **编译错误**: 确保安装了librt库
2. **权限错误**: 确保对目标目录有写权限
3. **磁盘损坏**: 使用`disk_format()`重新格式化
4. **性能问题**: 启用自动同步可能影响性能

### 调试建议

- 使用`disk_print_status()`查看磁盘状态
- 检查`disk_get_stats()`中的错误计数
- 使用`disk_error_to_string()`获取错误描述

## 许可证

本项目是课程设计的一部分，供教学和学习使用。 