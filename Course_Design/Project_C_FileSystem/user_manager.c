/**
 * User Management System Implementation
 * user_manager.c
 * 
 * 实现简单的用户管理系统，包括用户创建、登录、登出和权限检查
 */

#include "user_manager.h"
#include "fs_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*==============================================================================
 * 外部变量声明
 *============================================================================*/

extern fs_state_t g_fs_state;

/*==============================================================================
 * 内部辅助函数声明
 *============================================================================*/

static int find_user_by_username(const char* username);
static int find_user_by_uid(uint32_t uid);
static int find_free_user_slot(void);
static uint32_t get_next_available_uid(void);

/*==============================================================================
 * 用户管理函数实现
 *============================================================================*/

/**
 * 初始化用户管理系统
 */
user_error_t user_manager_init(void) {
    printf("初始化用户管理系统...\n");
    
    // 清空用户表
    memset(g_fs_state.users, 0, sizeof(g_fs_state.users));
    
    // 创建root用户
    user_error_t result = user_manager_create_user("root", "root123", 
                                                  USER_MANAGER_ROOT_UID, 
                                                  USER_MANAGER_ROOT_GID);
    if (result != USER_SUCCESS) {
        printf("创建root用户失败\n");
        return result;
    }
    
    // 创建匿名用户
    result = user_manager_create_user("anonymous", "", 
                                     USER_MANAGER_ANONYMOUS_UID, 
                                     USER_MANAGER_ANONYMOUS_UID);
    if (result != USER_SUCCESS) {
        printf("创建匿名用户失败\n");
        return result;
    }
    
    // 默认登录为root用户
    g_fs_state.current_user_uid = USER_MANAGER_ROOT_UID;
    
    printf("用户管理系统初始化完成\n");
    printf("  - root用户 (UID: %d)\n", USER_MANAGER_ROOT_UID);
    printf("  - anonymous用户 (UID: %d)\n", USER_MANAGER_ANONYMOUS_UID);
    printf("  - 当前用户: root\n");
    
    return USER_SUCCESS;
}

/**
 * 创建新用户
 */
user_error_t user_manager_create_user(const char* username, const char* password, 
                                     uint32_t uid, uint32_t gid) {
    if (!username || strlen(username) == 0) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    if (strlen(username) >= sizeof(g_fs_state.users[0].username)) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    // 检查用户名是否已存在
    if (find_user_by_username(username) >= 0) {
        return USER_ERROR_USER_EXISTS;
    }
    
    // 如果UID为0，自动分配
    if (uid == 0) {
        uid = get_next_available_uid();
        if (uid == 0) {
            return USER_ERROR_NO_SPACE;
        }
    } else {
        // 检查UID是否已存在
        if (find_user_by_uid(uid) >= 0) {
            return USER_ERROR_USER_EXISTS;
        }
    }
    
    // 如果GID为0，使用默认GID
    if (gid == 0) {
        gid = USER_MANAGER_DEFAULT_GID;
    }
    
    // 找到空闲的用户槽
    int slot = find_free_user_slot();
    if (slot < 0) {
        return USER_ERROR_NO_SPACE;
    }
    
    // 创建用户
    fs_user_t* user = &g_fs_state.users[slot];
    user->uid = uid;
    user->gid = gid;
    strncpy(user->username, username, sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    
    // 计算密码哈希
    if (password && strlen(password) > 0) {
        user_manager_hash_password(password, user->password_hash);
    } else {
        memset(user->password_hash, 0, sizeof(user->password_hash));
    }
    
    user->created_time = time(NULL);
    user->is_active = 1;
    
    printf("用户创建成功: %s (UID: %d, GID: %d)\n", username, uid, gid);
    
    return USER_SUCCESS;
}

/**
 * 用户登录
 */
user_error_t user_manager_login(const char* username, const char* password) {
    if (!username || !password) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    // 查找用户
    int slot = find_user_by_username(username);
    if (slot < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    fs_user_t* user = &g_fs_state.users[slot];
    
    // 检查用户是否激活
    if (!user->is_active) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // 验证密码
    char password_hash[64];
    user_manager_hash_password(password, password_hash);
    
    if (strcmp(user->password_hash, password_hash) != 0) {
        return USER_ERROR_WRONG_PASSWORD;
    }
    
    // 设置当前用户
    g_fs_state.current_user_uid = user->uid;
    
    printf("用户登录成功: %s (UID: %d)\n", username, user->uid);
    
    return USER_SUCCESS;
}

/**
 * 用户登出
 */
user_error_t user_manager_logout(void) {
    if (g_fs_state.current_user_uid == USER_MANAGER_ANONYMOUS_UID) {
        return USER_ERROR_NOT_LOGGED_IN;
    }
    
    printf("用户登出: UID %d\n", g_fs_state.current_user_uid);
    g_fs_state.current_user_uid = USER_MANAGER_ANONYMOUS_UID;
    
    return USER_SUCCESS;
}

/**
 * 获取当前登录用户信息
 */
user_error_t user_manager_get_current_user(fs_user_t* user_info) {
    if (!user_info) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    return user_manager_get_user_by_uid(g_fs_state.current_user_uid, user_info);
}

/**
 * 根据UID获取用户信息
 */
user_error_t user_manager_get_user_by_uid(uint32_t uid, fs_user_t* user_info) {
    if (!user_info) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    int slot = find_user_by_uid(uid);
    if (slot < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    memcpy(user_info, &g_fs_state.users[slot], sizeof(fs_user_t));
    return USER_SUCCESS;
}

/**
 * 根据用户名获取用户信息
 */
user_error_t user_manager_get_user_by_name(const char* username, fs_user_t* user_info) {
    if (!username || !user_info) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    int slot = find_user_by_username(username);
    if (slot < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    memcpy(user_info, &g_fs_state.users[slot], sizeof(fs_user_t));
    return USER_SUCCESS;
}

/**
 * 检查当前用户是否有指定权限
 */
int user_manager_check_permission(const fs_inode_t* inode, fs_permission_t required_perm) {
    if (!inode) {
        return 0;
    }
    
    fs_user_t current_user;
    if (user_manager_get_current_user(&current_user) != USER_SUCCESS) {
        return 0;
    }
    
    return user_manager_check_permission_detailed(current_user.uid, current_user.gid,
                                                 inode->owner_uid, inode->owner_gid,
                                                 inode->permissions, required_perm);
}

/**
 * 检查用户是否为超级用户
 */
int user_manager_is_root(uint32_t uid) {
    return (uid == USER_MANAGER_ROOT_UID);
}

/**
 * 获取当前用户UID
 */
uint32_t user_manager_get_current_uid(void) {
    return g_fs_state.current_user_uid;
}

/**
 * 获取当前用户GID
 */
uint32_t user_manager_get_current_gid(void) {
    fs_user_t current_user;
    if (user_manager_get_current_user(&current_user) != USER_SUCCESS) {
        return USER_MANAGER_ANONYMOUS_UID;
    }
    return current_user.gid;
}

/**
 * 设置文件所有者（仅超级用户可用）
 */
user_error_t user_manager_chown(uint32_t inode_number, uint32_t new_uid, uint32_t new_gid) {
    // 检查当前用户权限
    if (!user_manager_is_root(g_fs_state.current_user_uid)) {
        return USER_ERROR_PERMISSION;
    }
    
    // 读取inode
    fs_inode_t inode;
    fs_error_t result = fs_ops_read_inode(inode_number, &inode);
    if (result != FS_SUCCESS) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    // 修改所有者
    inode.owner_uid = new_uid;
    inode.owner_gid = new_gid;
    inode.change_time = time(NULL);
    
    // 写回inode
    result = fs_ops_write_inode(inode_number, &inode);
    if (result != FS_SUCCESS) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    printf("文件所有者已修改: inode %d -> UID %d, GID %d\n", 
           inode_number, new_uid, new_gid);
    
    return USER_SUCCESS;
}

/**
 * 修改文件权限
 */
user_error_t user_manager_chmod(uint32_t inode_number, uint16_t new_permissions) {
    // 读取inode
    fs_inode_t inode;
    fs_error_t result = fs_ops_read_inode(inode_number, &inode);
    if (result != FS_SUCCESS) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    // 检查权限：只有文件所有者或root可以修改权限
    uint32_t current_uid = g_fs_state.current_user_uid;
    if (!user_manager_is_root(current_uid) && current_uid != inode.owner_uid) {
        return USER_ERROR_PERMISSION;
    }
    
    // 修改权限
    inode.permissions = new_permissions;
    inode.change_time = time(NULL);
    
    // 写回inode
    result = fs_ops_write_inode(inode_number, &inode);
    if (result != FS_SUCCESS) {
        return USER_ERROR_INVALID_PARAM;
    }
    
    char perm_str[10];
    user_manager_permissions_to_string(new_permissions, perm_str);
    printf("文件权限已修改: inode %d -> %s (0%o)\n", 
           inode_number, perm_str, new_permissions);
    
    return USER_SUCCESS;
}

/**
 * 列出所有用户
 */
void user_manager_list_users(void) {
    printf("\n=== 用户列表 ===\n");
    printf("UID\tGID\t用户名\t\t状态\t创建时间\n");
    printf("---\t---\t------\t\t----\t--------\n");
    
    for (int i = 0; i < MAX_USERS; i++) {
        fs_user_t* user = &g_fs_state.users[i];
        if (user->is_active) {
            char time_str[20];
            struct tm* tm_info = localtime(&user->created_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            printf("%d\t%d\t%-12s\t%s\t%s\n", 
                   user->uid, user->gid, user->username, 
                   user->is_active ? "活跃" : "禁用", time_str);
        }
    }
    
    printf("\n当前登录用户: UID %d\n", g_fs_state.current_user_uid);
}

/**
 * 将权限转换为字符串
 */
void user_manager_permissions_to_string(uint16_t permissions, char* buffer) {
    if (!buffer) return;
    
    buffer[0] = (permissions & FS_PERM_OWNER_READ) ? 'r' : '-';
    buffer[1] = (permissions & FS_PERM_OWNER_WRITE) ? 'w' : '-';
    buffer[2] = (permissions & FS_PERM_OWNER_EXEC) ? 'x' : '-';
    buffer[3] = (permissions & FS_PERM_GROUP_READ) ? 'r' : '-';
    buffer[4] = (permissions & FS_PERM_GROUP_WRITE) ? 'w' : '-';
    buffer[5] = (permissions & FS_PERM_GROUP_EXEC) ? 'x' : '-';
    buffer[6] = (permissions & FS_PERM_OTHER_READ) ? 'r' : '-';
    buffer[7] = (permissions & FS_PERM_OTHER_WRITE) ? 'w' : '-';
    buffer[8] = (permissions & FS_PERM_OTHER_EXEC) ? 'x' : '-';
    buffer[9] = '\0';
}

/**
 * 计算简单密码哈希
 */
void user_manager_hash_password(const char* password, char* hash_buffer) {
    if (!password || !hash_buffer) return;
    
    // 简单的哈希算法（仅用于演示，实际应用中应使用更安全的算法）
    uint32_t hash = 5381;
    const char* str = password;
    
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    
    snprintf(hash_buffer, 64, "%08x", hash);
}

/**
 * 检查特定用户对文件的权限
 */
int user_manager_check_permission_detailed(uint32_t user_uid, uint32_t user_gid,
                                          uint32_t file_uid, uint32_t file_gid,
                                          uint16_t file_permissions, 
                                          fs_permission_t required_perm) {
    // 超级用户拥有所有权限
    if (user_manager_is_root(user_uid)) {
        return 1;
    }
    
    // 确定用户类别和所需权限
    fs_permission_t actual_perm;
    
    if (user_uid == file_uid) {
        // 文件所有者
        switch (required_perm) {
            case FS_PERM_OWNER_READ:
            case FS_PERM_GROUP_READ:
            case FS_PERM_OTHER_READ:
                actual_perm = FS_PERM_OWNER_READ;
                break;
            case FS_PERM_OWNER_WRITE:
            case FS_PERM_GROUP_WRITE:
            case FS_PERM_OTHER_WRITE:
                actual_perm = FS_PERM_OWNER_WRITE;
                break;
            case FS_PERM_OWNER_EXEC:
            case FS_PERM_GROUP_EXEC:
            case FS_PERM_OTHER_EXEC:
                actual_perm = FS_PERM_OWNER_EXEC;
                break;
            default:
                actual_perm = required_perm;
                break;
        }
    } else if (user_gid == file_gid) {
        // 同组用户
        switch (required_perm) {
            case FS_PERM_OWNER_READ:
            case FS_PERM_GROUP_READ:
            case FS_PERM_OTHER_READ:
                actual_perm = FS_PERM_GROUP_READ;
                break;
            case FS_PERM_OWNER_WRITE:
            case FS_PERM_GROUP_WRITE:
            case FS_PERM_OTHER_WRITE:
                actual_perm = FS_PERM_GROUP_WRITE;
                break;
            case FS_PERM_OWNER_EXEC:
            case FS_PERM_GROUP_EXEC:
            case FS_PERM_OTHER_EXEC:
                actual_perm = FS_PERM_GROUP_EXEC;
                break;
            default:
                actual_perm = required_perm;
                break;
        }
    } else {
        // 其他用户
        switch (required_perm) {
            case FS_PERM_OWNER_READ:
            case FS_PERM_GROUP_READ:
            case FS_PERM_OTHER_READ:
                actual_perm = FS_PERM_OTHER_READ;
                break;
            case FS_PERM_OWNER_WRITE:
            case FS_PERM_GROUP_WRITE:
            case FS_PERM_OTHER_WRITE:
                actual_perm = FS_PERM_OTHER_WRITE;
                break;
            case FS_PERM_OWNER_EXEC:
            case FS_PERM_GROUP_EXEC:
            case FS_PERM_OTHER_EXEC:
                actual_perm = FS_PERM_OTHER_EXEC;
                break;
            default:
                actual_perm = required_perm;
                break;
        }
    }
    
    return (file_permissions & actual_perm) != 0;
}

/*==============================================================================
 * 内部辅助函数实现
 *============================================================================*/

/**
 * 根据用户名查找用户
 */
static int find_user_by_username(const char* username) {
    for (int i = 0; i < MAX_USERS; i++) {
        fs_user_t* user = &g_fs_state.users[i];
        if (user->is_active && strcmp(user->username, username) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * 根据UID查找用户
 */
static int find_user_by_uid(uint32_t uid) {
    for (int i = 0; i < MAX_USERS; i++) {
        fs_user_t* user = &g_fs_state.users[i];
        if (user->is_active && user->uid == uid) {
            return i;
        }
    }
    return -1;
}

/**
 * 查找空闲的用户槽
 */
static int find_free_user_slot(void) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (!g_fs_state.users[i].is_active) {
            return i;
        }
    }
    return -1;
}

/**
 * 获取下一个可用的UID
 */
static uint32_t get_next_available_uid(void) {
    uint32_t uid = USER_MANAGER_DEFAULT_UID;
    
    while (uid < USER_MANAGER_ANONYMOUS_UID) {
        if (find_user_by_uid(uid) < 0) {
            return uid;
        }
        uid++;
    }
    
    return 0; // 没有可用的UID
} 