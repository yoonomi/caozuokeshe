/**
 * User Management and File Protection Test
 * user_protection_test.c
 * 
 * 测试用户管理系统和文件保护功能
 */

#include "user_manager.h"
#include "file_ops.h"
#include "fs_ops.h"
#include "disk_simulator.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 外部变量声明
extern fs_state_t g_fs_state;

#define TEST_DISK_FILE "user_protection_test.img"
#define TEST_DISK_SIZE (16 * 1024 * 1024)

void test_user_management(void);
void test_file_permissions(void);
void test_permission_enforcement(void);

int main(void) {
    printf("================ 用户管理和文件保护测试 ================\n");
    
    // 清理旧文件
    unlink(TEST_DISK_FILE);
    
    // 初始化文件系统
    printf("初始化文件系统...\n");
    int result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("磁盘初始化失败\n");
        return 1;
    }
    
    format_disk();
    
    // 关闭并重新打开
    disk_close();
    result = disk_init(TEST_DISK_FILE, TEST_DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("重新打开磁盘失败\n");
        return 1;
    }
    
    // 初始化用户管理系统
    printf("\n初始化用户管理系统...\n");
    user_error_t user_result = user_manager_init();
    if (user_result != USER_SUCCESS) {
        printf("用户管理系统初始化失败\n");
        return 1;
    }
    
    // 运行测试
    test_user_management();
    test_file_permissions();
    test_permission_enforcement();
    
    // 清理
    disk_close();
    unlink(TEST_DISK_FILE);
    
    printf("\n================ 测试完成 ================\n");
    return 0;
}

void test_user_management(void) {
    printf("\n=== 测试 1: 用户管理功能 ===\n");
    
    // 创建普通用户
    printf("创建普通用户...\n");
    user_error_t result = user_manager_create_user("alice", "alice123", 1001, 1001);
    printf("创建用户alice: %s\n", result == USER_SUCCESS ? "成功" : "失败");
    
    result = user_manager_create_user("bob", "bob456", 1002, 1002);
    printf("创建用户bob: %s\n", result == USER_SUCCESS ? "成功" : "失败");
    
    // 列出所有用户
    user_manager_list_users();
    
    // 测试用户登录
    printf("\n测试用户登录...\n");
    result = user_manager_login("alice", "alice123");
    printf("alice登录: %s\n", result == USER_SUCCESS ? "成功" : "失败");
    
    // 获取当前用户信息
    fs_user_t current_user;
    result = user_manager_get_current_user(&current_user);
    if (result == USER_SUCCESS) {
        printf("当前用户: %s (UID: %d)\n", current_user.username, current_user.uid);
    }
    
    // 测试错误密码
    result = user_manager_login("bob", "wrongpassword");
    printf("错误密码登录: %s\n", result == USER_ERROR_WRONG_PASSWORD ? "正确拒绝" : "意外结果");
    
    printf("用户管理功能测试完成\n");
}

void test_file_permissions(void) {
    printf("\n=== 测试 2: 文件权限管理 ===\n");
    
    // 以root用户登录
    user_manager_login("root", "root123");
    printf("当前用户: root\n");
    
    // 创建文件
    printf("创建测试文件...\n");
    int result = fs_create("test_perm.txt");
    printf("创建文件: %s\n", result == FS_SUCCESS ? "成功" : "失败");
    
    if (result == FS_SUCCESS) {
        // 打开文件
        int fd = fs_open("test_perm.txt");
        if (fd >= 0) {
            // 写入数据
            const char* data = "This is a test file for permissions.";
            int bytes = fs_write(fd, data, strlen(data));
            printf("写入数据: %d 字节\n", bytes);
            
            fs_close(fd);
        }
        
        // 修改文件权限为只读（对于其他用户）
        printf("\n修改文件权限...\n");
        
        // 首先需要找到文件的inode号
        fd = fs_open("test_perm.txt");
        if (fd >= 0) {
            fs_file_handle_t *handle = &g_fs_state.open_files[fd];
            uint32_t inode_num = handle->inode_number;
            fs_close(fd);
            
            // 修改权限为 rw-r--r-- (644)
            user_error_t perm_result = user_manager_chmod(inode_num, 0644);
            printf("修改权限为644: %s\n", perm_result == USER_SUCCESS ? "成功" : "失败");
            
            // 修改所有者为alice
            perm_result = user_manager_chown(inode_num, 1001, 1001);
            printf("修改所有者为alice: %s\n", perm_result == USER_SUCCESS ? "成功" : "失败");
        }
    }
    
    printf("文件权限管理测试完成\n");
}

void test_permission_enforcement(void) {
    printf("\n=== 测试 3: 权限强制执行 ===\n");
    
    // 切换到alice用户
    printf("切换到alice用户...\n");
    user_error_t result = user_manager_login("alice", "alice123");
    printf("alice登录: %s\n", result == USER_SUCCESS ? "成功" : "失败");
    
    if (result == USER_SUCCESS) {
        // 尝试打开自己的文件（应该成功）
        printf("\n测试访问自己的文件...\n");
        int fd = fs_open("test_perm.txt");
        printf("打开自己的文件: %s\n", fd >= 0 ? "成功" : "失败");
        
        if (fd >= 0) {
            // 尝试读取（应该成功）
            char buffer[100];
            int bytes = fs_read(fd, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf("读取内容: [%s]\n", buffer);
            } else {
                printf("读取失败: %d\n", bytes);
            }
            
            // 尝试写入（应该成功，因为是文件所有者）
            const char* new_data = "\nAlice's modification.";
            bytes = fs_write(fd, new_data, strlen(new_data));
            printf("写入数据: %s (%d 字节)\n", bytes > 0 ? "成功" : "失败", bytes);
            
            fs_close(fd);
        }
        
        // 创建一个新文件（应该以alice为所有者）
        printf("\n创建alice的文件...\n");
        int create_result = fs_create("alice_file.txt");
        printf("创建alice_file.txt: %s\n", create_result == FS_SUCCESS ? "成功" : "失败");
    }
    
    // 切换到bob用户
    printf("\n切换到bob用户...\n");
    result = user_manager_login("bob", "bob456");
    printf("bob登录: %s\n", result == USER_SUCCESS ? "成功" : "失败");
    
    if (result == USER_SUCCESS) {
        // 尝试打开alice的文件（应该只能读取）
        printf("\n测试访问alice的文件...\n");
        int fd = fs_open("test_perm.txt");
        printf("打开alice的文件: %s\n", fd >= 0 ? "成功" : "失败");
        
        if (fd >= 0) {
            // 尝试读取（应该成功，因为有其他用户读权限）
            char buffer[100];
            int bytes = fs_read(fd, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                printf("读取内容: 成功 (%d 字节)\n", bytes);
            } else {
                printf("读取失败: %d\n", bytes);
            }
            
            // 尝试写入（应该失败，因为没有写权限）
            const char* bob_data = "\nBob's attempt to modify.";
            bytes = fs_write(fd, bob_data, strlen(bob_data));
            printf("写入尝试: %s (%d)\n", bytes > 0 ? "意外成功" : "正确失败", bytes);
            
            fs_close(fd);
        }
        
        // 尝试打开alice的私有文件
        printf("\n尝试访问alice_file.txt...\n");
        fd = fs_open("alice_file.txt");
        printf("打开alice_file.txt: %s\n", fd >= 0 ? "成功" : "失败");
        if (fd >= 0) {
            fs_close(fd);
        }
    }
    
    // 切换回root用户测试管理员权限
    printf("\n切换回root用户...\n");
    result = user_manager_login("root", "root123");
    printf("root登录: %s\n", result == USER_SUCCESS ? "成功" : "失败");
    
    if (result == USER_SUCCESS) {
        printf("\n测试root用户权限...\n");
        int fd = fs_open("test_perm.txt");
        printf("root打开任意文件: %s\n", fd >= 0 ? "成功" : "失败");
        
        if (fd >= 0) {
            // root应该能读写任何文件
            char buffer[200];
            int bytes = fs_read(fd, buffer, sizeof(buffer) - 1);
            printf("root读取: %s (%d 字节)\n", bytes > 0 ? "成功" : "失败", bytes);
            
            const char* root_data = "\nRoot access granted.";
            bytes = fs_write(fd, root_data, strlen(root_data));
            printf("root写入: %s (%d 字节)\n", bytes > 0 ? "成功" : "失败", bytes);
            
            fs_close(fd);
        }
    }
    
    printf("权限强制执行测试完成\n");
} 