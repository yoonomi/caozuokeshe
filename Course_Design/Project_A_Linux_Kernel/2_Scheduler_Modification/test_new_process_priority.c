/**
 * test_new_process_priority.c - Test program for new process priority boost
 * 
 * This program tests whether new processes get higher priority than
 * existing processes by creating child processes under CPU load.
 * 
 * Author: Yomi
 * Date: 2025-06-20
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#define NUM_CPU_HOGS 4
#define NUM_TEST_PROCESSES 3
#define TEST_DURATION_SECONDS 10

static volatile int keep_running = 1;

/**
 * Signal handler to stop the test
 */
void stop_test(int sig) {
    keep_running = 0;
    printf("\nStopping test...\n");
}

/**
 * Get current timestamp in microseconds
 */
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

/**
 * CPU-intensive task that consumes CPU cycles
 */
void cpu_hog_task() {
    volatile long counter = 0;
    printf("CPU hog process %d started\n", getpid());
    
    while (keep_running) {
        for (int i = 0; i < 1000000; i++) {
            counter++;
        }
    }
    
    printf("CPU hog process %d finished (counter: %ld)\n", getpid(), counter);
    exit(0);
}

/**
 * Test process that measures how quickly it gets scheduled
 */
void test_process(int process_id, long long creation_time) {
    long long first_run_time = get_time_us();
    long long delay = first_run_time - creation_time;
    
    printf("Test process %d (PID: %d) got CPU after %lld microseconds\n", 
           process_id, getpid(), delay);
    
    /* Do some work to show it's running */
    volatile long work_counter = 0;
    long long start_work = get_time_us();
    
    while (keep_running && (get_time_us() - start_work) < 2000000) { /* Run for 2 seconds */
        for (int i = 0; i < 100000; i++) {
            work_counter++;
        }
    }
    
    printf("Test process %d completed work (counter: %ld)\n", process_id, work_counter);
    exit(0);
}

/**
 * Create CPU-intensive background load
 */
void create_cpu_load() {
    printf("Creating %d CPU-intensive background processes...\n", NUM_CPU_HOGS);
    
    for (int i = 0; i < NUM_CPU_HOGS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* Child process becomes CPU hog */
            cpu_hog_task();
        } else if (pid > 0) {
            printf("Created CPU hog process: PID %d\n", pid);
        } else {
            perror("Failed to create CPU hog process");
        }
    }
    
    /* Let the CPU hogs settle in */
    sleep(2);
    printf("CPU load established, system should be busy\n");
}

/**
 * Main test function
 */
int main() {
    pid_t test_pids[NUM_TEST_PROCESSES];
    long long creation_times[NUM_TEST_PROCESSES];
    
    printf("===========================================\n");
    printf("  New Process Priority Boost Test\n");
    printf("===========================================\n");
    printf("Test duration: %d seconds\n", TEST_DURATION_SECONDS);
    printf("CPU hogs: %d processes\n", NUM_CPU_HOGS);
    printf("Test processes: %d processes\n", NUM_TEST_PROCESSES);
    printf("===========================================\n");
    
    /* Set up signal handler */
    signal(SIGALRM, stop_test);
    signal(SIGINT, stop_test);
    
    /* Create CPU-intensive background load */
    create_cpu_load();
    
    printf("\nStarting new process priority test...\n");
    printf("Expected behavior: New processes should get CPU quickly despite high load\n\n");
    
    /* Set test duration */
    alarm(TEST_DURATION_SECONDS);
    
    /* Create test processes with delays to see scheduling behavior */
    for (int i = 0; i < NUM_TEST_PROCESSES; i++) {
        creation_times[i] = get_time_us();
        
        test_pids[i] = fork();
        if (test_pids[i] == 0) {
            /* Child process runs the test */
            test_process(i + 1, creation_times[i]);
        } else if (test_pids[i] > 0) {
            printf("Created test process %d: PID %d\n", i + 1, test_pids[i]);
        } else {
            perror("Failed to create test process");
            break;
        }
        
        /* Wait a bit between process creation to observe behavior */
        usleep(500000); /* 0.5 seconds */
    }
    
    printf("\nAll test processes created. Waiting for test completion...\n");
    
    /* Wait for all processes to complete */
    int status;
    while (wait(&status) > 0) {
        /* Collect all child processes */
    }
    
    printf("\n===========================================\n");
    printf("  Test completed\n");
    printf("===========================================\n");
    printf("If the patch works correctly, you should see:\n");
    printf("1. Test processes getting CPU quickly despite load\n");
    printf("2. Kernel messages about first-time process runs\n");
    printf("3. Lower scheduling delays for new processes\n");
    printf("\nCheck kernel log with: dmesg | grep SCHED\n");
    printf("===========================================\n");
    
    return 0;
} 