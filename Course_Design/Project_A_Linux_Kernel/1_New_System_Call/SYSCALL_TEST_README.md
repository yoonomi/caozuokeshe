# 系统调用测试程序使用指南

## 文件说明

本目录包含了测试自定义系统调用 `sys_mykernelexec` 的完整工具集：

- **test_syscall.c** - 用户空间测试程序源码
- **test_syscall** - 编译后的可执行文件
- **Makefile** - 自动化编译脚本
- **test_demo.sh** - 演示脚本

## 编译程序

### 方法1：使用Makefile
```bash
make
```

### 方法2：直接编译
```bash
gcc -Wall -Wextra -std=c99 -g -o test_syscall test_syscall.c
```

## 运行测试

### 基本测试
```bash
./test_syscall
```

### 多次调用测试
```bash
# 默认3次调用
./test_syscall -m

# 指定调用次数 (1-10次)
./test_syscall -m 5
```

### 查看帮助
```bash
./test_syscall --help
```

## 程序功能特性

### 🔍 详细的系统信息显示
- 进程ID、父进程ID
- 用户ID、组ID
- 当前时间戳

### 🧪 完整的测试功能
- 系统调用号 250 的测试
- 返回值检查和错误分析
- 多次调用性能测试

### 📊 智能错误分析
程序会识别常见错误类型：
- `ENOSYS (38)` - 系统调用未实现
- `EPERM (1)` - 权限不足
- `EINVAL (22)` - 无效参数
- 其他未知错误

### 📝 内核日志指导
程序会提供查看内核日志的具体命令：
```bash
# 查看最近的内核消息
dmesg | tail -10

# 实时监控内核消息
dmesg -w

# 使用systemctl查看日志
journalctl -f -k
```

## 预期运行结果

### 当前系统（系统调用未实现）
```
✗ 状态: 失败 (FAILED)
✗ 错误码: 95
✗ 错误描述: Operation not supported
✗ 可能原因: 未知错误
```

### 修改内核后（系统调用已实现）
```
✓ 状态: 成功 (SUCCESS)
✓ 系统调用正常执行
✓ 内核消息已写入日志
```

## 系统调用实现要求

要使测试成功，需要在Linux内核中实现：

1. **系统调用号定义** (`include/asm-i386/unistd.h`)
   ```c
   #define __NR_mykernelexec 250
   ```

2. **系统调用实现** (`kernel/sys.c`)
   ```c
   asmlinkage long sys_mykernelexec(void)
   {
       printk(KERN_INFO "[Yomi] says hello from the kernel!\n");
       printk(KERN_INFO "sys_mykernelexec called by process %s (PID: %d)\n", 
              current->comm, current->pid);
       return 0;
   }
   ```

3. **系统调用表更新** (`arch/i386/kernel/entry.S`)
   ```assembly
   .long sys_mykernelexec    /* 250 */
   ```

## 测试验证步骤

1. **编译并运行测试程序**
   ```bash
   make test
   ```

2. **实时监控内核日志**
   ```bash
   # 在另一个终端窗口运行
   dmesg -w
   ```

3. **执行系统调用测试**
   ```bash
   ./test_syscall
   ```

4. **检查内核日志输出**
   应该看到类似输出：
   ```
   [Yomi] says hello from the kernel!
   sys_mykernelexec called by process test_syscall (PID: XXXX)
   ```

## 常见问题

### Q: 为什么总是显示"失败"？
A: 这是正常的！因为我们还没有在内核中实际添加这个系统调用。错误码95 (Operation not supported) 表示系统调用号250不存在。

### Q: 如何确认系统调用成功添加？
A: 运行测试程序后，应该看到返回值为0，并且dmesg中会显示我们的内核消息。

### Q: 可以修改系统调用号吗？
A: 可以，但需要同时修改test_syscall.c中的 `__NR_mykernelexec` 定义和内核实现中的系统调用号。

## 技术细节

- **系统调用接口**: 使用标准的 `syscall()` 函数
- **错误处理**: 完整的errno错误码分析
- **性能测试**: 支持多次调用性能测量
- **跨平台**: 兼容Linux x86/x86_64架构

---

*作者: Yomi*  
*日期: 2025-06-20*  
*项目: Linux内核系统调用添加演示* 