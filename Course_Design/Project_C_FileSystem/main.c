/**
 * 文件系统交互式命令行Shell
 * main.c
 * 
 * 提供用户友好的命令行界面来操作文件系统
 */

#include "user_manager.h"
#include "file_ops.h"
#include "fs_ops.h"
#include "disk_simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_LENGTH 512
#define MAX_ARGS 10
#define DISK_FILE "filesystem.img"
#define DISK_SIZE (32 * 1024 * 1024)  // 32MB

// 全局状态
static int shell_running = 1;
static int system_initialized = 0;

// 函数声明
void show_welcome(void);
void show_help(void);
void show_prompt(void);
int parse_command(char *input, char *args[]);
int execute_command(int argc, char *args[]);
void cleanup_and_exit(void);

// 命令处理函数
int cmd_help(int argc, char *args[]);
int cmd_exit(int argc, char *args[]);
int cmd_init(int argc, char *args[]);
int cmd_format(int argc, char *args[]);
int cmd_status(int argc, char *args[]);

// 用户管理命令
int cmd_login(int argc, char *args[]);
int cmd_logout(int argc, char *args[]);
int cmd_adduser(int argc, char *args[]);
int cmd_whoami(int argc, char *args[]);
int cmd_users(int argc, char *args[]);
int cmd_chmod(int argc, char *args[]);
int cmd_chown(int argc, char *args[]);

// 文件操作命令
int cmd_create(int argc, char *args[]);
int cmd_open(int argc, char *args[]);
int cmd_close(int argc, char *args[]);
int cmd_read(int argc, char *args[]);
int cmd_write(int argc, char *args[]);
int cmd_seek(int argc, char *args[]);
int cmd_tell(int argc, char *args[]);
int cmd_size(int argc, char *args[]);
int cmd_ls(int argc, char *args[]);

// 命令表结构
typedef struct {
    const char *name;
    int (*handler)(int argc, char *args[]);
    const char *usage;
    const char *description;
} command_t;

// 命令表
static const command_t commands[] = {
    // 系统命令
    {"help",     cmd_help,     "help",                    "显示帮助信息"},
    {"exit",     cmd_exit,     "exit",                    "退出程序"},
    {"quit",     cmd_exit,     "quit",                    "退出程序"},
    {"init",     cmd_init,     "init",                    "初始化文件系统"},
    {"format",   cmd_format,   "format",                  "格式化文件系统"},
    {"status",   cmd_status,   "status",                  "显示系统状态"},
    
    // 用户管理命令
    {"login",    cmd_login,    "login <username> <password>", "用户登录"},
    {"logout",   cmd_logout,   "logout",                  "用户登出"},
    {"adduser",  cmd_adduser,  "adduser <username> <password> [uid] [gid]", "添加用户"},
    {"whoami",   cmd_whoami,   "whoami",                  "显示当前用户"},
    {"users",    cmd_users,    "users",                   "列出所有用户"},
    {"chmod",    cmd_chmod,    "chmod <fd> <permissions>", "修改文件权限"},
    {"chown",    cmd_chown,    "chown <fd> <uid> <gid>",  "修改文件所有者"},
    
    // 文件操作命令
    {"create",   cmd_create,   "create <filename>",       "创建新文件"},
    {"open",     cmd_open,     "open <filename>",         "打开文件"},
    {"close",    cmd_close,    "close <fd>",              "关闭文件"},
    {"read",     cmd_read,     "read <fd> <bytes>",       "读取文件"},
    {"write",    cmd_write,    "write <fd> <text>",       "写入文件"},
    {"seek",     cmd_seek,     "seek <fd> <offset> <whence>", "移动文件指针"},
    {"tell",     cmd_tell,     "tell <fd>",               "获取文件指针位置"},
    {"size",     cmd_size,     "size <fd>",               "获取文件大小"},
    {"ls",       cmd_ls,       "ls",                      "列出打开的文件"},
    
    {NULL, NULL, NULL, NULL}  // 结束标记
};

/*==============================================================================
 * 主要界面函数
 *============================================================================*/

void show_welcome(void) {
    printf("\n");
    printf("┌─────────────────────────────────────────────────────────┐\n");
    printf("│              文件系统模拟器 v2.0                       │\n");
    printf("│           Filesystem Simulator with User Management    │\n");
    printf("│                                                         │\n");
    printf("│  特性: 多用户支持、权限控制、文件读写                   │\n");
    printf("│                                                         │\n");
    printf("│  输入 'help' 查看所有可用命令                           │\n");
    printf("│  输入 'init' 初始化文件系统                             │\n");
    printf("│  输入 'exit' 退出程序                                   │\n");
    printf("└─────────────────────────────────────────────────────────┘\n");
    printf("\n");
}

void show_help(void) {
    printf("\n=== 文件系统命令帮助 ===\n\n");
    
    printf("系统命令:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strstr(commands[i].description, "系统") != NULL || 
            strcmp(commands[i].name, "help") == 0 ||
            strcmp(commands[i].name, "exit") == 0 ||
            strcmp(commands[i].name, "quit") == 0 ||
            strcmp(commands[i].name, "init") == 0 ||
            strcmp(commands[i].name, "format") == 0 ||
            strcmp(commands[i].name, "status") == 0) {
            printf("  %-12s - %s\n", commands[i].usage, commands[i].description);
        }
    }
    
    printf("\n用户管理命令:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strstr(commands[i].description, "用户") != NULL) {
            printf("  %-35s - %s\n", commands[i].usage, commands[i].description);
        }
    }
    
    printf("\n文件操作命令:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strstr(commands[i].description, "文件") != NULL) {
            printf("  %-35s - %s\n", commands[i].usage, commands[i].description);
        }
    }
    
    printf("\n权限说明:\n");
    printf("  权限格式: 八进制数字 (如 644, 755)\n");
    printf("  644 = rw-r--r-- (所有者读写，其他只读)\n");
    printf("  755 = rwxr-xr-x (所有者全权限，其他读执行)\n");
    
    printf("\nwhence参数 (用于seek命令):\n");
    printf("  0 = SEEK_SET (从文件开头)\n");
    printf("  1 = SEEK_CUR (从当前位置)\n");
    printf("  2 = SEEK_END (从文件末尾)\n");
    
    printf("\n=======================================\n");
}

void show_prompt(void) {
    if (system_initialized) {
        fs_user_t current_user;
        if (user_manager_get_current_user(&current_user) == USER_SUCCESS) {
            printf("[%s@fs]$ ", current_user.username);
        } else {
            printf("[unknown@fs]$ ");
        }
    } else {
        printf("[未初始化]$ ");
    }
    fflush(stdout);
}

/*==============================================================================
 * 命令解析和执行
 *============================================================================*/

int parse_command(char *input, char *args[]) {
    int argc = 0;
    char *token = strtok(input, " \t\n");
    
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    args[argc] = NULL;
    return argc;
}

int execute_command(int argc, char *args[]) {
    if (argc == 0) {
        return 0;
    }
    
    // 查找命令
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(args[0], commands[i].name) == 0) {
            return commands[i].handler(argc, args);
        }
    }
    
    printf("未知命令: %s\n", args[0]);
    printf("输入 'help' 查看所有可用命令\n");
    return 0;
}

void cleanup_and_exit(void) {
    if (system_initialized) {
        printf("正在清理资源...\n");
        user_manager_logout();
        disk_close();
    }
    printf("再见！\n");
}

/*==============================================================================
 * 系统命令实现
 *============================================================================*/

int cmd_help(int argc, char *args[]) {
    (void)argc; (void)args;  // 避免未使用参数警告
    show_help();
    return 0;
}

int cmd_exit(int argc, char *args[]) {
    (void)argc; (void)args;
    shell_running = 0;
    return 0;
}

int cmd_init(int argc, char *args[]) {
    (void)argc; (void)args;
    
    if (system_initialized) {
        printf("文件系统已经初始化\n");
        return 0;
    }
    
    printf("正在初始化文件系统...\n");
    
    // 初始化磁盘
    int result = disk_init(DISK_FILE, DISK_SIZE);
    if (result != DISK_SUCCESS) {
        printf("磁盘初始化失败: %s\n", disk_error_to_string(result));
        return -1;
    }
    
    // 检查是否需要格式化
    printf("检查文件系统格式...\n");
    
    // 尝试读取超级块来检查是否已格式化
    char buffer[1024];
    result = disk_read_block(0, buffer);
    if (result != DISK_SUCCESS) {
        printf("需要格式化文件系统\n");
        format_disk();
    } else {
        // 检查魔数
        fs_superblock_t *sb = (fs_superblock_t*)buffer;
        if (sb->magic_number != FS_MAGIC_NUMBER) {
            printf("文件系统格式无效，需要重新格式化\n");
            format_disk();
        } else {
            printf("文件系统格式有效\n");
        }
    }
    
    // 初始化用户管理系统
    printf("初始化用户管理系统...\n");
    user_error_t user_result = user_manager_init();
    if (user_result != USER_SUCCESS) {
        printf("用户管理系统初始化失败\n");
        return -1;
    }
    
    system_initialized = 1;
    printf("文件系统初始化完成！\n");
    printf("提示: 使用 'login root root123' 以管理员身份登录\n");
    
    return 0;
}

int cmd_format(int argc, char *args[]) {
    (void)argc; (void)args;
    
    printf("警告: 这将清除所有数据！\n");
    printf("确认格式化文件系统吗? (y/N): ");
    fflush(stdout);
    
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL ||
        (response[0] != 'y' && response[0] != 'Y')) {
        printf("操作已取消\n");
        return 0;
    }
    
    printf("正在格式化文件系统...\n");
    format_disk();
    
    // 重新初始化用户管理系统
    if (system_initialized) {
        user_manager_init();
    }
    
    printf("格式化完成！\n");
    return 0;
}

int cmd_status(int argc, char *args[]) {
    (void)argc; (void)args;
    
    if (!system_initialized) {
        printf("文件系统未初始化\n");
        return 0;
    }
    
    fs_ops_print_status();
    return 0;
}

/*==============================================================================
 * 用户管理命令实现
 *============================================================================*/

int cmd_login(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统 (使用 'init' 命令)\n");
        return 0;
    }
    
    if (argc < 3) {
        printf("用法: login <username> <password>\n");
        return 0;
    }
    
    user_error_t result = user_manager_login(args[1], args[2]);
    if (result == USER_SUCCESS) {
        printf("登录成功\n");
    } else {
        printf("登录失败: ");
        switch (result) {
            case USER_ERROR_USER_NOT_FOUND:
                printf("用户不存在\n");
                break;
            case USER_ERROR_WRONG_PASSWORD:
                printf("密码错误\n");
                break;
            default:
                printf("未知错误\n");
                break;
        }
    }
    
    return 0;
}

int cmd_logout(int argc, char *args[]) {
    (void)argc; (void)args;
    
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    user_error_t result = user_manager_logout();
    if (result == USER_SUCCESS) {
        printf("登出成功\n");
    } else {
        printf("登出失败\n");
    }
    
    return 0;
}

int cmd_adduser(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 3) {
        printf("用法: adduser <username> <password> [uid] [gid]\n");
        return 0;
    }
    
    uint32_t uid = (argc > 3) ? atoi(args[3]) : 0;  // 0表示自动分配
    uint32_t gid = (argc > 4) ? atoi(args[4]) : 0;  // 0表示使用默认值
    
    user_error_t result = user_manager_create_user(args[1], args[2], uid, gid);
    if (result == USER_SUCCESS) {
        printf("用户创建成功\n");
    } else {
        printf("用户创建失败: ");
        switch (result) {
            case USER_ERROR_USER_EXISTS:
                printf("用户已存在\n");
                break;
            case USER_ERROR_NO_SPACE:
                printf("用户表已满\n");
                break;
            case USER_ERROR_INVALID_PARAM:
                printf("参数无效\n");
                break;
            default:
                printf("未知错误\n");
                break;
        }
    }
    
    return 0;
}

int cmd_whoami(int argc, char *args[]) {
    (void)argc; (void)args;
    
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    fs_user_t current_user;
    user_error_t result = user_manager_get_current_user(&current_user);
    if (result == USER_SUCCESS) {
        printf("当前用户: %s (UID: %d, GID: %d)\n", 
               current_user.username, current_user.uid, current_user.gid);
    } else {
        printf("未登录或获取用户信息失败\n");
    }
    
    return 0;
}

int cmd_users(int argc, char *args[]) {
    (void)argc; (void)args;
    
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    user_manager_list_users();
    return 0;
}

int cmd_chmod(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 3) {
        printf("用法: chmod <fd> <permissions>\n");
        printf("示例: chmod 0 644\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    int permissions = strtol(args[2], NULL, 8);  // 八进制
    
    // 从文件描述符获取inode号
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        printf("无效的文件描述符: %d\n", fd);
        return 0;
    }
    
    // 这里需要从打开文件表获取inode号
    extern fs_state_t g_fs_state;
    uint32_t inode_num = g_fs_state.open_files[fd].inode_number;
    
    user_error_t user_result = user_manager_chmod(inode_num, permissions);
    if (user_result == USER_SUCCESS) {
        printf("权限修改成功\n");
    } else {
        printf("权限修改失败: 权限不足或参数错误\n");
    }
    
    return 0;
}

int cmd_chown(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 4) {
        printf("用法: chown <fd> <uid> <gid>\n");
        printf("示例: chown 0 1001 1001\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    uint32_t uid = atoi(args[2]);
    uint32_t gid = atoi(args[3]);
    
    // 从文件描述符获取inode号
    fs_error_t result = file_ops_validate_fd(fd);
    if (result != FS_SUCCESS) {
        printf("无效的文件描述符: %d\n", fd);
        return 0;
    }
    
    extern fs_state_t g_fs_state;
    uint32_t inode_num = g_fs_state.open_files[fd].inode_number;
    
    user_error_t user_result = user_manager_chown(inode_num, uid, gid);
    if (user_result == USER_SUCCESS) {
        printf("所有者修改成功\n");
    } else {
        printf("所有者修改失败: 权限不足或参数错误\n");
    }
    
    return 0;
}

/*==============================================================================
 * 文件操作命令实现
 *============================================================================*/

int cmd_create(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 2) {
        printf("用法: create <filename>\n");
        return 0;
    }
    
    int result = fs_create(args[1]);
    if (result == FS_SUCCESS) {
        printf("文件创建成功: %s\n", args[1]);
    } else {
        printf("文件创建失败: %s\n", fs_ops_error_to_string(result));
    }
    
    return 0;
}

int cmd_open(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 2) {
        printf("用法: open <filename>\n");
        return 0;
    }
    
    int fd = fs_open(args[1]);
    if (fd >= 0) {
        printf("文件打开成功: %s (fd: %d)\n", args[1], fd);
    } else {
        printf("文件打开失败: %s\n", fs_ops_error_to_string(fd));
    }
    
    return 0;
}

int cmd_close(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 2) {
        printf("用法: close <fd>\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    fs_close(fd);
    printf("文件描述符 %d 已关闭\n", fd);
    
    return 0;
}

int cmd_read(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 3) {
        printf("用法: read <fd> <bytes>\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    int bytes = atoi(args[2]);
    
    if (bytes <= 0 || bytes > 1024) {
        printf("字节数必须在1-1024之间\n");
        return 0;
    }
    
    char *buffer = malloc(bytes + 1);
    if (!buffer) {
        printf("内存分配失败\n");
        return 0;
    }
    
    int result = fs_read(fd, buffer, bytes);
    if (result > 0) {
        buffer[result] = '\0';
        printf("读取了 %d 字节:\n", result);
        printf("内容: [%s]\n", buffer);
    } else if (result == 0) {
        printf("已到达文件末尾\n");
    } else {
        printf("读取失败: %s\n", fs_ops_error_to_string(result));
    }
    
    free(buffer);
    return 0;
}

int cmd_write(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 3) {
        printf("用法: write <fd> <text>\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    
    // 合并所有剩余参数作为文本内容
    char text[512] = "";
    for (int i = 2; i < argc; i++) {
        if (i > 2) strcat(text, " ");
        strcat(text, args[i]);
    }
    
    int result = fs_write(fd, text, strlen(text));
    if (result > 0) {
        printf("写入了 %d 字节\n", result);
    } else {
        printf("写入失败: %s\n", fs_ops_error_to_string(result));
    }
    
    return 0;
}

int cmd_seek(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 4) {
        printf("用法: seek <fd> <offset> <whence>\n");
        printf("whence: 0=SEEK_SET, 1=SEEK_CUR, 2=SEEK_END\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    int offset = atoi(args[2]);
    int whence = atoi(args[3]);
    
    int result = fs_seek(fd, offset, whence);
    if (result >= 0) {
        printf("文件指针移动成功，新位置: %d\n", result);
    } else {
        printf("文件指针移动失败: %s\n", fs_ops_error_to_string(result));
    }
    
    return 0;
}

int cmd_tell(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 2) {
        printf("用法: tell <fd>\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    int position = fs_tell(fd);
    
    if (position >= 0) {
        printf("当前文件指针位置: %d\n", position);
    } else {
        printf("获取位置失败: %s\n", fs_ops_error_to_string(position));
    }
    
    return 0;
}

int cmd_size(int argc, char *args[]) {
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    if (argc < 2) {
        printf("用法: size <fd>\n");
        return 0;
    }
    
    int fd = atoi(args[1]);
    int file_size = fs_size(fd);
    
    if (file_size >= 0) {
        printf("文件大小: %d 字节\n", file_size);
    } else {
        printf("获取文件大小失败: %s\n", fs_ops_error_to_string(file_size));
    }
    
    return 0;
}

int cmd_ls(int argc, char *args[]) {
    (void)argc; (void)args;
    
    if (!system_initialized) {
        printf("请先初始化文件系统\n");
        return 0;
    }
    
    printf("\n=== 当前打开的文件 ===\n");
    printf("FD\tInode\t位置\t\t所有者\n");
    printf("--\t-----\t----\t\t------\n");
    
    extern fs_state_t g_fs_state;
    int found = 0;
    
    for (int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (g_fs_state.open_files[fd].reference_count > 0) {
            fs_file_handle_t *handle = &g_fs_state.open_files[fd];
            printf("%d\t%u\t%lu\t\t%u\n", 
                   fd, handle->inode_number, 
                   handle->file_position, handle->owner_uid);
            found = 1;
        }
    }
    
    if (!found) {
        printf("没有打开的文件\n");
    }
    
    printf("====================\n");
    return 0;
}

/*==============================================================================
 * 主程序
 *============================================================================*/

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;  // 避免未使用参数警告
    
    char input[MAX_INPUT_LENGTH];
    char *args[MAX_ARGS];
    int arg_count;
    
    // 显示欢迎信息
    show_welcome();
    
    // 主命令循环
    while (shell_running) {
        show_prompt();
        
        // 读取用户输入
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // 移除末尾的换行符
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        // 跳过空行
        if (strlen(input) == 0) {
            continue;
        }
        
        // 解析和执行命令
        arg_count = parse_command(input, args);
        if (arg_count > 0) {
            execute_command(arg_count, args);
        }
    }
    
    // 清理资源
    cleanup_and_exit();
    
    return 0;
} 