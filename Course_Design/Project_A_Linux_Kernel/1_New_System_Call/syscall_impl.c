/**
 * 新系统调用的内核实现
 * syscall_impl.c
 * 
 * 这个文件包含新系统调用的核心实现代码
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/string.h>

/**
 * 自定义系统调用: sys_my_syscall
 * 功能: 演示一个简单的系统调用实现
 * 参数: 
 *   - arg1: 输入参数1
 *   - arg2: 输入参数2
 *   - result: 输出结果缓冲区
 * 返回值: 成功返回0，失败返回错误码
 */
SYSCALL_DEFINE3(my_syscall, int, arg1, int, arg2, char __user *, result)
{
    char kernel_buffer[256];
    int sum = arg1 + arg2;
    
    // 格式化结果字符串
    snprintf(kernel_buffer, sizeof(kernel_buffer), 
             "System call executed: %d + %d = %d\n", arg1, arg2, sum);
    
    // 将结果从内核空间复制到用户空间
    if (copy_to_user(result, kernel_buffer, strlen(kernel_buffer) + 1)) {
        return -EFAULT;
    }
    
    printk(KERN_INFO "New system call executed: %d + %d = %d\n", arg1, arg2, sum);
    
    return 0;
}

/**
 * 注册系统调用的相关说明:
 * 1. 在 arch/x86/entry/syscalls/syscall_64.tbl 中添加系统调用号
 * 2. 在 include/linux/syscalls.h 中声明函数原型
 * 3. 重新编译内核
 */ 