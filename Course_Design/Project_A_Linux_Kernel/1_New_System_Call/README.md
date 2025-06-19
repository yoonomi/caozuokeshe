# 新系统调用实现

## 项目概述
本项目演示如何在Linux内核中添加一个新的系统调用，包括内核实现和用户空间测试程序。

## 文件说明
- `syscall_impl.c`: 新系统调用的内核实现
- `user_test.c`: 用户空间测试程序
- `README.md`: 本说明文档

## 系统调用功能
实现了一个简单的系统调用 `my_syscall`，该系统调用接收两个整数参数，计算它们的和，并将结果返回给用户空间。

## 编译和安装步骤

### 1. 内核修改
#### 1.1 添加系统调用实现
将 `syscall_impl.c` 中的代码添加到内核源码中，通常放在 `kernel/` 目录下。

#### 1.2 修改系统调用表
在 `arch/x86/entry/syscalls/syscall_64.tbl` 文件中添加一行：
```
335	common	my_syscall		__x64_sys_my_syscall
```

#### 1.3 添加函数声明
在 `include/linux/syscalls.h` 文件中添加：
```c
asmlinkage long sys_my_syscall(int arg1, int arg2, char __user *result);
```

### 2. 编译内核
```bash
# 配置内核
make menuconfig

# 编译内核
make -j$(nproc)

# 安装模块
sudo make modules_install

# 安装内核
sudo make install

# 更新引导加载程序
sudo update-grub
```

### 3. 重启系统
```bash
sudo reboot
```

### 4. 编译测试程序
```bash
gcc -o user_test user_test.c
```

### 5. 运行测试
```bash
./user_test
```

## 测试说明
测试程序会：
1. 提示用户输入两个整数
2. 调用新的系统调用进行计算
3. 显示计算结果
4. 执行一系列自动测试案例

## 验证系统调用
可以通过以下方式验证系统调用是否正确安装：
1. 检查 `/proc/kallsyms` 中是否包含系统调用符号
2. 使用 `dmesg` 查看内核日志输出
3. 运行测试程序验证功能

## 注意事项
- 确保系统调用号（335）没有被其他系统调用使用
- 在生产环境中使用前请充分测试
- 修改内核具有风险，建议在虚拟机中进行实验

## 常见问题
1. **编译错误**: 检查内核版本兼容性
2. **系统调用号冲突**: 选择未使用的系统调用号
3. **权限问题**: 确保有足够的权限进行内核编译和安装 