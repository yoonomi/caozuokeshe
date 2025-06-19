/**
 * test_syscall.c - 用户空间测试程序
 * 
 * 功能：测试新增的 sys_mykernelexec 系统调用
 * 作者：Yomi
 * 日期：2025-06-20
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>
#include <time.h>

/* 定义新系统调用号 */
#define __NR_mykernelexec 250

/**
 * 封装系统调用的便捷函数
 * 
 * @return 系统调用的返回值
 */
long mykernelexec(void) {
    return syscall(__NR_mykernelexec);
}

/**
 * 打印程序头部信息
 */
void print_header(void) {
    printf("==========================================\n");
    printf("    自定义系统调用测试程序\n");
    printf("    Testing sys_mykernelexec syscall\n");
    printf("==========================================\n");
}

/**
 * 打印当前进程信息
 */
void print_process_info(void) {
    printf("进程信息:\n");
    printf("  进程ID (PID): %d\n", getpid());
    printf("  父进程ID (PPID): %d\n", getppid());
    printf("  用户ID (UID): %d\n", getuid());
    printf("  有效用户ID (EUID): %d\n", geteuid());
    printf("  组ID (GID): %d\n", getgid());
    
    /* 获取当前时间 */
    time_t current_time = time(NULL);
    printf("  当前时间: %s", ctime(&current_time));
    printf("\n");
}

/**
 * 测试系统调用并显示结果
 */
void test_syscall(void) {
    long result;
    
    printf("正在调用系统调用...\n");
    printf("系统调用号: %d\n", __NR_mykernelexec);
    printf("调用函数: syscall(%d)\n\n", __NR_mykernelexec);
    
    /* 执行系统调用 */
    printf("执行中: mykernelexec()\n");
    result = mykernelexec();
    
    /* 检查结果并显示状态 */
    printf("\n系统调用执行完成!\n");
    printf("返回值: %ld\n", result);
    
    if (result == 0) {
        printf("✓ 状态: 成功 (SUCCESS)\n");
        printf("✓ 系统调用正常执行\n");
        printf("✓ 内核消息已写入日志\n");
    } else if (result == -1) {
        printf("✗ 状态: 失败 (FAILED)\n");
        printf("✗ 错误码: %d\n", errno);
        printf("✗ 错误描述: %s\n", strerror(errno));
        
        /* 提供可能的错误原因分析 */
        switch (errno) {
            case ENOSYS:
                printf("✗ 可能原因: 系统调用未实现或内核版本不匹配\n");
                break;
            case EPERM:
                printf("✗ 可能原因: 权限不足\n");
                break;
            case EINVAL:
                printf("✗ 可能原因: 无效的系统调用号\n");
                break;
            default:
                printf("✗ 可能原因: 未知错误\n");
                break;
        }
    } else {
        printf("? 状态: 意外返回值\n");
        printf("? 返回值不是预期的0或-1\n");
    }
}

/**
 * 提供查看内核日志的指导
 */
void print_log_instructions(void) {
    printf("\n==========================================\n");
    printf("查看内核日志指令:\n");
    printf("==========================================\n");
    printf("1. 查看最近的内核消息:\n");
    printf("   dmesg | tail -10\n\n");
    printf("2. 实时监控内核消息:\n");
    printf("   dmesg -w\n\n");
    printf("3. 查看系统日志文件:\n");
    printf("   tail -f /var/log/messages\n");
    printf("   tail -f /var/log/kern.log\n\n");
    printf("4. 使用journalctl (systemd系统):\n");
    printf("   journalctl -f -k\n\n");
    printf("预期的内核输出应包含:\n");
    printf("  '[Yomi] says hello from the kernel!'\n");
    printf("==========================================\n");
}

/**
 * 多次调用测试
 */
void multiple_calls_test(int count) {
    printf("\n==========================================\n");
    printf("多次调用测试 (共%d次)\n", count);
    printf("==========================================\n");
    
    for (int i = 1; i <= count; i++) {
        printf("第 %d 次调用: ", i);
        fflush(stdout);  /* 确保输出立即显示 */
        
        long result = mykernelexec();
        
        if (result == 0) {
            printf("成功\n");
        } else {
            printf("失败 (返回值: %ld, errno: %d)\n", result, errno);
        }
        
        /* 在调用之间稍作延迟，便于观察内核日志 */
        if (i < count) {
            usleep(100000);  /* 延迟100毫秒 */
        }
    }
    
    printf("多次调用测试完成!\n");
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
    int multiple_test = 0;
    int call_count = 3;
    
    /* 解析命令行参数 */
    if (argc > 1) {
        if (strcmp(argv[1], "-m") == 0 || strcmp(argv[1], "--multiple") == 0) {
            multiple_test = 1;
            if (argc > 2) {
                call_count = atoi(argv[2]);
                if (call_count <= 0 || call_count > 10) {
                    printf("警告: 调用次数应在1-10之间，使用默认值3\n");
                    call_count = 3;
                }
            }
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("用法: %s [选项]\n", argv[0]);
            printf("选项:\n");
            printf("  -h, --help           显示此帮助信息\n");
            printf("  -m, --multiple [N]   执行多次调用测试 (默认3次)\n");
            printf("\n示例:\n");
            printf("  %s                   单次测试\n", argv[0]);
            printf("  %s -m               多次测试 (3次)\n", argv[0]);
            printf("  %s -m 5             多次测试 (5次)\n", argv[0]);
            return 0;
        }
    }
    
    /* 显示程序信息 */
    print_header();
    print_process_info();
    
    /* 执行基本测试 */
    test_syscall();
    
    /* 如果请求多次测试 */
    if (multiple_test) {
        multiple_calls_test(call_count);
    }
    
    /* 显示日志查看指导 */
    print_log_instructions();
    
    return 0;
} 