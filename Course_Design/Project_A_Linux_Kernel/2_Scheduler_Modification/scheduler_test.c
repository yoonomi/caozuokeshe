/**
 * scheduler_test.c - Linux 2.4 新进程优先级提升测试程序
 * 
 * 功能：验证新进程是否获得了优先级提升
 * 作者：Yomi
 * 日期：2025-06-20
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define MAX_CHILDREN 10
#define TEST_DURATION 5     /* 秒 */
#define CPU_INTENSIVE_LOOPS 1000000

/* 全局变量 */
static volatile int test_running = 1;
static volatile int children_created = 0;
static volatile int children_finished = 0;

/**
 * 信号处理函数
 */
void sigalrm_handler(int sig) {
    test_running = 0;
    printf("\n=== 测试时间结束 ===\n");
}

void sigchld_handler(int sig) {
    children_finished++;
}

/**
 * 获取当前时间戳（微秒）
 */
long long get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

/**
 * CPU密集型任务
 */
void cpu_intensive_task(int child_id, long long start_time) {
    long long first_cpu_time = 0;
    int got_cpu = 0;
    volatile long counter = 0;
    
    printf("子进程 %d (PID: %d) 开始执行\n", child_id, getpid());
    
    while (test_running) {
        /* 执行CPU密集型计算 */
        for (int i = 0; i < CPU_INTENSIVE_LOOPS; i++) {
            counter++;
        }
        
        /* 记录首次获得CPU的时间 */
        if (!got_cpu) {
            first_cpu_time = get_timestamp_us();
            got_cpu = 1;
            
            long long delay = first_cpu_time - start_time;
            printf("子进程 %d 首次获得CPU，延迟: %lld 微秒\n", 
                   child_id, delay);
        }
    }
    
    printf("子进程 %d 完成，总计算量: %ld\n", child_id, counter);
    exit(0);
}

/**
 * 创建CPU密集型背景负载
 */
void create_background_load(int num_processes) {
    printf("创建 %d 个背景负载进程...\n", num_processes);
    
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程：无限循环消耗CPU */
            volatile long counter = 0;
            while (1) {
                for (int j = 0; j < 100000; j++) {
                    counter++;
                }
            }
        } else if (pid > 0) {
            printf("创建背景负载进程 PID: %d\n", pid);
        } else {
            perror("创建背景负载失败");
        }
    }
    
    /* 让背景负载稳定运行 */
    sleep(1);
    printf("背景负载已稳定运行\n");
}

/**
 * 测试新进程调度延迟
 */
void test_new_process_scheduling() {
    pid_t children[MAX_CHILDREN];
    long long creation_times[MAX_CHILDREN];
    int num_children = 5;
    
    printf("\n=== 开始新进程调度延迟测试 ===\n");
    printf("将在高CPU负载下创建 %d 个新进程\n", num_children);
    
    /* 设置信号处理 */
    signal(SIGALRM, sigalrm_handler);
    signal(SIGCHLD, sigchld_handler);
    
    /* 设置测试时长 */
    alarm(TEST_DURATION);
    
    /* 创建测试进程 */
    for (int i = 0; i < num_children; i++) {
        creation_times[i] = get_timestamp_us();
        
        children[i] = fork();
        if (children[i] == 0) {
            /* 子进程执行CPU密集型任务 */
            cpu_intensive_task(i + 1, creation_times[i]);
        } else if (children[i] > 0) {
            children_created++;
            printf("创建测试进程 %d (PID: %d)\n", i + 1, children[i]);
            
            /* 间隔创建，观察调度行为 */
            usleep(200000); /* 200ms间隔 */
        } else {
            perror("创建子进程失败");
            break;
        }
    }
    
    printf("已创建 %d 个测试进程，等待测试完成...\n", children_created);
    
    /* 等待测试完成 */
    while (test_running) {
        sleep(1);
        printf("测试进行中... (已完成进程: %d/%d)\n", 
               children_finished, children_created);
    }
    
    /* 清理子进程 */
    for (int i = 0; i < children_created; i++) {
        if (children[i] > 0) {
            kill(children[i], SIGTERM);
            waitpid(children[i], NULL, 0);
        }
    }
}

/**
 * 显示系统调度统计信息
 */
void show_scheduler_stats() {
    FILE *fp;
    char buffer[256];
    
    printf("\n=== 调度器统计信息 ===\n");
    
    /* 读取新进程调度统计 */
    fp = fopen("/proc/sys/kernel/new_process_scheduled", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            printf("新进程调度总数: %s", buffer);
        }
        fclose(fp);
    }
    
    fp = fopen("/proc/sys/kernel/boost_applied_count", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            printf("优先级提升应用次数: %s", buffer);
        }
        fclose(fp);
    }
    
    /* 显示当前调度参数 */
    printf("\n=== 当前调度参数 ===\n");
    
    fp = fopen("/proc/sys/kernel/new_process_boost", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp)) {
            printf("新进程加成值: %s", buffer);
        }
        fclose(fp);
    } else {
        printf("注意: 无法读取调度参数，可能未应用补丁\n");
    }
}

/**
 * 调整调度参数测试
 */
void test_scheduler_tuning() {
    FILE *fp;
    int original_boost = 50;
    
    printf("\n=== 调度参数调优测试 ===\n");
    
    /* 读取原始值 */
    fp = fopen("/proc/sys/kernel/new_process_boost", "r");
    if (fp) {
        fscanf(fp, "%d", &original_boost);
        fclose(fp);
        printf("原始新进程加成值: %d\n", original_boost);
    }
    
    /* 尝试修改参数 */
    fp = fopen("/proc/sys/kernel/new_process_boost", "w");
    if (fp) {
        fprintf(fp, "%d", 80);
        fclose(fp);
        printf("尝试设置新进程加成值为: 80\n");
        
        /* 验证修改 */
        fp = fopen("/proc/sys/kernel/new_process_boost", "r");
        if (fp) {
            int new_value;
            fscanf(fp, "%d", &new_value);
            fclose(fp);
            printf("实际设置值: %d\n", new_value);
        }
        
        /* 恢复原始值 */
        fp = fopen("/proc/sys/kernel/new_process_boost", "w");
        if (fp) {
            fprintf(fp, "%d", original_boost);
            fclose(fp);
            printf("已恢复原始值: %d\n", original_boost);
        }
    } else {
        printf("无法修改调度参数，可能需要root权限\n");
    }
}

/**
 * 进程信息显示测试
 */
void test_process_info() {
    char filename[64];
    FILE *fp;
    char buffer[1024];
    
    printf("\n=== 进程调度信息测试 ===\n");
    
    /* 检查当前进程的调度信息 */
    snprintf(filename, sizeof(filename), "/proc/%d/status", getpid());
    fp = fopen(filename, "r");
    if (fp) {
        printf("当前进程 (PID: %d) 的调度信息:\n", getpid());
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, "HasRunBefore") || 
                strstr(buffer, "RunCount") || 
                strstr(buffer, "FirstRunTime") || 
                strstr(buffer, "NewProcessBoost")) {
                printf("  %s", buffer);
            }
        }
        fclose(fp);
    } else {
        printf("无法读取进程状态信息，可能未应用补丁\n");
    }
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
    int background_load = 0;
    int show_help = 0;
    
    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--background") == 0) {
            background_load = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help = 1;
        }
    }
    
    if (show_help) {
        printf("用法: %s [选项]\n", argv[0]);
        printf("选项:\n");
        printf("  -b, --background    创建背景CPU负载\n");
        printf("  -h, --help         显示帮助信息\n");
        printf("\n功能:\n");
        printf("  测试Linux 2.4新进程优先级提升机制\n");
        printf("  验证新创建进程是否获得调度优势\n");
        return 0;
    }
    
    printf("=========================================\n");
    printf("   Linux 2.4 新进程调度优化测试\n");
    printf("=========================================\n");
    printf("PID: %d\n", getpid());
    printf("测试时间: %d 秒\n", TEST_DURATION);
    printf("=========================================\n");
    
    /* 显示当前调度器状态 */
    show_scheduler_stats();
    
    /* 测试进程信息显示 */
    test_process_info();
    
    /* 如果需要，创建背景负载 */
    if (background_load) {
        create_background_load(3);
    }
    
    /* 执行主要测试 */
    test_new_process_scheduling();
    
    /* 显示最终统计 */
    printf("\n=== 测试完成 ===\n");
    show_scheduler_stats();
    
    /* 测试参数调优 */
    test_scheduler_tuning();
    
    printf("\n测试程序结束\n");
    printf("建议查看内核日志: dmesg | grep SCHED\n");
    
    return 0;
} 