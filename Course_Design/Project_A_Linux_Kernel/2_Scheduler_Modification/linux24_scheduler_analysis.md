# Linux 2.4 调度器分析与修改方案

## 1. goodness() 函数分析

### 函数作用
`goodness()` 函数是Linux 2.4调度器的核心，负责计算每个可运行进程的"良好度"值，调度器基于此值选择下一个要运行的进程。

### 原始goodness()函数实现 (kernel/sched.c)
```c
static inline int goodness(struct task_struct * p, int this_cpu, struct mm_struct *this_mm)
{
    int weight;

    /*
     * 选择当前进程的情况
     * Giving the process a big priority boost
     */
    if (p->processor == this_cpu)
        weight = 1000 + p->counter;
    else
        weight = p->counter;

    if (weight) {
        if (p->mm == this_mm || !p->mm)
            weight += 1;
        weight += 20 - p->nice;
    }
    return weight;
}
```

### 详细分析

#### 1.1 函数参数
- `p`: 待评估的进程task_struct指针
- `this_cpu`: 当前CPU编号
- `this_mm`: 当前进程的内存管理结构

#### 1.2 计算逻辑

**基础权重计算:**
```c
if (p->processor == this_cpu)
    weight = 1000 + p->counter;    // CPU亲和性优化
else
    weight = p->counter;           // 基于时间片的权重
```

**权重调整因子:**
```c
if (weight) {
    if (p->mm == this_mm || !p->mm)
        weight += 1;               // 内存管理优化
    weight += 20 - p->nice;        // nice值影响
}
```

#### 1.3 关键因素分析

1. **CPU亲和性 (CPU Affinity)**
   - 如果进程上次在当前CPU运行，给予1000的巨大加成
   - 目的：减少缓存失效，提高性能

2. **时间片计数器 (p->counter)**
   - 反映进程剩余时间片
   - 时间片越多，优先级越高

3. **内存管理优化**
   - 共享内存管理结构的进程获得+1加成
   - 内核线程(!p->mm)也获得此优化

4. **Nice值影响**
   - `20 - p->nice` 范围：0-39
   - nice值越低，优先级越高

#### 1.4 权重值含义
- **weight = 0**: 进程时间片耗尽，不可调度
- **weight > 0**: 可调度，值越大优先级越高
- **weight > 1000**: 具有CPU亲和性的高优先级进程

### 1.5 调度器选择逻辑
```c
// 在schedule()函数中
list_for_each(tmp, &runqueue_head) {
    p = list_entry(tmp, struct task_struct, run_list);
    if (can_schedule(p, this_cpu)) {
        int weight = goodness(p, this_cpu, prev->active_mm);
        if (weight > c)
            c = weight, next = p;
    }
}
```

调度器遍历运行队列，选择goodness值最高的进程作为下一个运行的进程。

---

## 2. 新进程优先级提升实现方案

### 2.1 设计目标
使新创建的普通进程获得比已运行进程更高的优先级，确保新进程能够及时获得CPU时间。

### 2.2 核心思想
在task_struct中添加`has_run_before`标志，修改goodness()函数为新创建进程提供优先级加成。

### 2.3 详细实现方案

#### 步骤1: 修改task_struct结构 (include/linux/sched.h)
```c
struct task_struct {
    // ... 现有字段 ...
    
    // 新增字段：进程是否已经运行过
    unsigned int has_run_before:1;      // 位域，节省内存
    unsigned long first_run_time;       // 首次运行时间戳
    unsigned int run_count;             // 运行次数计数器
    
    // ... 其他字段 ...
};
```

#### 步骤2: 修改进程创建函数 (kernel/fork.c)
```c
// 在copy_process()函数中初始化新字段
static struct task_struct *copy_process(unsigned long clone_flags, ...)
{
    // ... 现有代码 ...
    
    // 初始化新进程标志
    p->has_run_before = 0;           // 标记为未运行
    p->first_run_time = 0;           // 初始化首次运行时间
    p->run_count = 0;                // 初始化运行计数
    
    // ... 其他初始化代码 ...
}
```

#### 步骤3: 修改goodness()函数 (kernel/sched.c)
```c
static inline int goodness(struct task_struct * p, int this_cpu, struct mm_struct *this_mm)
{
    int weight;
    
    /*
     * CPU亲和性优化
     */
    if (p->processor == this_cpu)
        weight = 1000 + p->counter;
    else
        weight = p->counter;

    if (weight) {
        /*
         * 新进程优先级提升机制
         * 给未运行过的新进程额外的优先级加成
         */
        if (!p->has_run_before) {
            weight += NEW_PROCESS_BOOST;     // 新进程加成
        }
        
        /*
         * 渐进式优先级衰减
         * 基于运行次数逐渐降低新进程优势
         */
        if (p->run_count < MAX_BOOST_RUNS) {
            weight += (MAX_BOOST_RUNS - p->run_count) * DECAY_FACTOR;
        }
        
        // 原有的优化
        if (p->mm == this_mm || !p->mm)
            weight += 1;
        weight += 20 - p->nice;
    }
    return weight;
}
```

#### 步骤4: 修改调度切换函数 (kernel/sched.c)
```c
asmlinkage void schedule(void)
{
    struct task_struct *prev, *next;
    struct list_head *tmp;
    int this_cpu, c;
    
    // ... 现有调度逻辑 ...
    
    /*
     * 更新即将运行进程的状态
     */
    if (next && next != prev) {
        if (!next->has_run_before) {
            next->has_run_before = 1;
            next->first_run_time = jiffies;
            printk(KERN_INFO "First run: process %d (%s)\n", 
                   next->pid, next->comm);
        }
        next->run_count++;
    }
    
    // ... 继续调度逻辑 ...
}
```

### 2.4 配置参数定义
```c
// 在kernel/sched.c顶部定义可调参数
#define NEW_PROCESS_BOOST    50      // 新进程基础加成
#define MAX_BOOST_RUNS       5       // 最大优惠运行次数
#define DECAY_FACTOR         10      // 优先级衰减因子
#define MIN_NEW_TIMESLICE    10      // 新进程最小时间片
```

### 2.5 增强功能实现

#### 时间片管理优化
```c
// 在schedule()中为新进程提供更大的初始时间片
if (!next->has_run_before && next->counter < MIN_NEW_TIMESLICE) {
    next->counter = MIN_NEW_TIMESLICE;
}
```

#### 统计信息收集
```c
// 在kernel/sched.c中添加统计变量
static unsigned long new_process_scheduled = 0;
static unsigned long boost_applied_count = 0;

// 在goodness()函数中更新统计
if (!p->has_run_before) {
    boost_applied_count++;
}
```

#### /proc接口支持
```c
// 在fs/proc/array.c中添加新字段显示
static inline int task_state(struct task_struct *p, char *buffer)
{
    // ... 现有代码 ...
    
    // 添加新进程状态信息
    res += sprintf(buffer + res,
        "HasRunBefore:\t%d\n"
        "RunCount:\t%u\n"
        "FirstRunTime:\t%lu\n",
        p->has_run_before,
        p->run_count,
        p->first_run_time);
    
    return res;
}
```

---

## 3. 实现效果分析

### 3.1 预期效果
1. **新进程响应性**: 新创建进程能更快获得CPU时间
2. **交互体验**: 用户启动程序的响应速度显著提升
3. **系统公平性**: 通过渐进衰减保持长期公平性

### 3.2 性能影响
- **开销**: 每次goodness()调用增加几个简单比较
- **内存**: 每个task_struct增加约8字节
- **收益**: 显著改善新进程启动延迟

### 3.3 调优建议
```c
// 可通过sysctl动态调整的参数
static int new_process_boost = 50;
static int max_boost_runs = 5;
static int decay_factor = 10;
```

---

## 4. 测试验证方案

### 4.1 功能测试程序
```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main() {
    struct timespec start, end;
    pid_t pid;
    
    printf("测试新进程调度优化...\n");
    
    // 创建CPU密集型背景负载
    for (int i = 0; i < 4; i++) {
        if (fork() == 0) {
            while (1) ; // 无限循环
        }
    }
    
    sleep(1); // 让背景负载稳定
    
    // 测试新进程启动时间
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    pid = fork();
    if (pid == 0) {
        // 子进程立即退出，测量获得CPU的时间
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("新进程获得CPU时间: %ld ns\n", 
               (end.tv_sec - start.tv_sec) * 1000000000 + 
               (end.tv_nsec - start.tv_nsec));
        exit(0);
    }
    
    wait(NULL);
    return 0;
}
```

### 4.2 性能对比测试
```bash
# 编译测试程序
gcc -o sched_test sched_test.c

# 运行对比测试
echo "原版内核测试:"
./sched_test

echo "修改版内核测试:"
./sched_test
```

---

## 5. 安全考虑

### 5.1 防止滥用
```c
// 限制fork炸弹攻击
#define MAX_NEW_PROCESS_PER_SEC 100

static unsigned long last_check_time = 0;
static unsigned int new_process_count = 0;

// 在copy_process()中添加检查
if (jiffies - last_check_time > HZ) {
    last_check_time = jiffies;
    new_process_count = 0;
}

if (++new_process_count > MAX_NEW_PROCESS_PER_SEC) {
    // 禁用新进程加成
    p->has_run_before = 1;
}
```

### 5.2 资源限制
- 确保新进程优势不会导致老进程饥饿
- 实现公平性保证机制
- 添加系统负载感知调整

---

这个实现方案在保持Linux 2.4调度器稳定性的同时，有效提升了新进程的响应性，是一个平衡性能与公平性的优雅解决方案。 