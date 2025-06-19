/**
 * 用户空间测试程序
 * user_test.c
 * 
 * 用于测试新系统调用的用户程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

// 定义新系统调用号（需要根据实际分配的号码修改）
#define SYS_MY_SYSCALL 335

/**
 * 调用自定义系统调用的wrapper函数
 */
long my_syscall(int arg1, int arg2, char *result) {
    return syscall(SYS_MY_SYSCALL, arg1, arg2, result);
}

int main() {
    char result[256];
    int arg1, arg2;
    long ret;
    
    printf("=== 新系统调用测试程序 ===\n");
    printf("请输入两个整数进行测试:\n");
    
    printf("输入第一个数字: ");
    scanf("%d", &arg1);
    
    printf("输入第二个数字: ");
    scanf("%d", &arg2);
    
    // 调用新的系统调用
    memset(result, 0, sizeof(result));
    ret = my_syscall(arg1, arg2, result);
    
    if (ret == 0) {
        printf("系统调用执行成功!\n");
        printf("结果: %s", result);
    } else {
        printf("系统调用执行失败，错误码: %ld\n", ret);
        perror("my_syscall");
    }
    
    // 进行多次测试
    printf("\n=== 自动测试 ===\n");
    int test_cases[][2] = {{1, 2}, {10, 20}, {100, 200}, {-5, 15}};
    int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_tests; i++) {
        memset(result, 0, sizeof(result));
        ret = my_syscall(test_cases[i][0], test_cases[i][1], result);
        
        if (ret == 0) {
            printf("测试 %d: %s", i+1, result);
        } else {
            printf("测试 %d 失败，错误码: %ld\n", i+1, ret);
        }
    }
    
    return 0;
}

/*
编译说明:
gcc -o user_test user_test.c

运行说明:
./user_test

注意: 确保内核已经加载了新的系统调用
*/ 