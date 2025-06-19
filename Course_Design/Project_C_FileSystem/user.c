/**
 * 用户管理模块
 * user.c
 * 
 * 处理用户创建、登录、权限验证等功能
 */

#include "fs.h"

/* 初始化用户管理模块 */
fs_error_t user_init(void) {
    // 清空用户表
    memset(g_fs.users, 0, sizeof(g_fs.users));
    
    // 创建默认root用户 (UID = 1)
    g_fs.users[1].uid = 1;
    strcpy(g_fs.users[1].username, "root");
    strcpy(g_fs.users[1].password, "root123");
    g_fs.users[1].gid = 0;  // root组
    g_fs.users[1].created_time = fs_current_time();
    g_fs.users[1].is_active = 1;
    
    g_fs.current_user = 0;  // 当前未登录状态
    
    printf("用户管理模块初始化完成，默认root用户已创建\n");
    return FS_SUCCESS;
}

/* 创建新用户 */
fs_error_t user_create(const char *username, const char *password, uint32_t gid) {
    if (!username || !password) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    if (strlen(username) >= MAX_FILENAME_LEN || strlen(password) >= MAX_FILENAME_LEN) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 检查用户名是否已存在
    for (int i = 1; i < MAX_USERS; i++) {
        if (g_fs.users[i].is_active && strcmp(g_fs.users[i].username, username) == 0) {
            return FS_ERROR_USER_EXISTS;
        }
    }
    
    // 查找空闲的用户槽位
    int free_slot = -1;
    for (int i = 1; i < MAX_USERS; i++) {
        if (!g_fs.users[i].is_active) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot == -1) {
        return FS_ERROR_NO_SPACE;
    }
    
    // 创建新用户
    user_t *new_user = &g_fs.users[free_slot];
    new_user->uid = free_slot;
    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    new_user->gid = gid;
    new_user->created_time = fs_current_time();
    new_user->is_active = 1;
    
    return FS_SUCCESS;
}

/* 用户登录 */
fs_error_t user_login(const char *username, const char *password) {
    if (!username || !password) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 查找用户
    for (int i = 1; i < MAX_USERS; i++) {
        if (g_fs.users[i].is_active && 
            strcmp(g_fs.users[i].username, username) == 0) {
            
            // 验证密码
            if (strcmp(g_fs.users[i].password, password) == 0) {
                g_fs.current_user = i;
                return FS_SUCCESS;
            } else {
                return FS_ERROR_PERMISSION;
            }
        }
    }
    
    return FS_ERROR_USER_NOT_FOUND;
}

/* 用户登出 */
fs_error_t user_logout(void) {
    if (g_fs.current_user == 0) {
        return FS_ERROR_INVALID_USER;
    }
    
    g_fs.current_user = 0;
    return FS_SUCCESS;
}

/* 检查用户权限 */
int user_check_permission(uint32_t uid, inode_t *inode, permission_t perm) {
    if (!inode) {
        return 0;
    }
    
    // root用户拥有所有权限
    if (uid == 1) {
        return 1;
    }
    
    // 检查文件所有者权限
    if (uid == inode->owner_uid) {
        // 检查所有者权限位（高3位）
        uint16_t owner_perms = (inode->permissions >> 6) & 0x7;
        return (owner_perms & perm) == perm;
    }
    
    // 检查组权限
    user_t *user = &g_fs.users[uid];
    if (user->is_active && user->gid == inode->owner_gid) {
        // 检查组权限位（中3位）
        uint16_t group_perms = (inode->permissions >> 3) & 0x7;
        return (group_perms & perm) == perm;
    }
    
    // 检查其他用户权限（低3位）
    uint16_t other_perms = inode->permissions & 0x7;
    return (other_perms & perm) == perm;
}

/* 获取当前用户信息 */
user_t* user_get_current(void) {
    if (g_fs.current_user == 0 || !g_fs.users[g_fs.current_user].is_active) {
        return NULL;
    }
    return &g_fs.users[g_fs.current_user];
}

/* 根据UID获取用户信息 */
user_t* user_get_by_uid(uint32_t uid) {
    if (uid >= MAX_USERS || !g_fs.users[uid].is_active) {
        return NULL;
    }
    return &g_fs.users[uid];
}

/* 列出所有用户 */
void user_list_all(void) {
    printf("系统用户列表:\n");
    printf("%-5s %-16s %-5s %-20s %-10s\n", "UID", "用户名", "GID", "创建时间", "状态");
    printf("-------------------------------------------------------------\n");
    
    for (int i = 1; i < MAX_USERS; i++) {
        if (g_fs.users[i].is_active) {
            char time_str[32];
            struct tm *tm_info = localtime(&g_fs.users[i].created_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            printf("%-5d %-16s %-5d %-20s %-10s\n",
                   g_fs.users[i].uid,
                   g_fs.users[i].username,
                   g_fs.users[i].gid,
                   time_str,
                   (i == g_fs.current_user) ? "当前用户" : "离线");
        }
    }
}

/* 删除用户 */
fs_error_t user_delete(const char *username) {
    if (!username) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 只有root可以删除用户
    if (g_fs.current_user != 1) {
        return FS_ERROR_PERMISSION;
    }
    
    // 查找要删除的用户
    for (int i = 2; i < MAX_USERS; i++) {  // 不能删除root用户
        if (g_fs.users[i].is_active && 
            strcmp(g_fs.users[i].username, username) == 0) {
            
            // 标记为非活跃状态
            g_fs.users[i].is_active = 0;
            
            // 如果删除的是当前用户，则自动登出
            if (i == g_fs.current_user) {
                g_fs.current_user = 0;
            }
            
            return FS_SUCCESS;
        }
    }
    
    return FS_ERROR_USER_NOT_FOUND;
}

/* 修改用户密码 */
fs_error_t user_change_password(const char *username, const char *old_password, 
                               const char *new_password) {
    if (!username || !old_password || !new_password) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    if (strlen(new_password) >= MAX_FILENAME_LEN) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 查找用户
    for (int i = 1; i < MAX_USERS; i++) {
        if (g_fs.users[i].is_active && 
            strcmp(g_fs.users[i].username, username) == 0) {
            
            // 验证旧密码（除非是root用户修改其他用户密码）
            if (g_fs.current_user != 1 && strcmp(g_fs.users[i].password, old_password) != 0) {
                return FS_ERROR_PERMISSION;
            }
            
            // 设置新密码
            strcpy(g_fs.users[i].password, new_password);
            return FS_SUCCESS;
        }
    }
    
    return FS_ERROR_USER_NOT_FOUND;
}

/* 获取用户权限字符串表示 */
void user_get_permission_string(uint16_t permissions, char *perm_str) {
    if (!perm_str) return;
    
    // 所有者权限
    perm_str[0] = (permissions & 0400) ? 'r' : '-';
    perm_str[1] = (permissions & 0200) ? 'w' : '-';
    perm_str[2] = (permissions & 0100) ? 'x' : '-';
    
    // 组权限
    perm_str[3] = (permissions & 0040) ? 'r' : '-';
    perm_str[4] = (permissions & 0020) ? 'w' : '-';
    perm_str[5] = (permissions & 0010) ? 'x' : '-';
    
    // 其他用户权限
    perm_str[6] = (permissions & 0004) ? 'r' : '-';
    perm_str[7] = (permissions & 0002) ? 'w' : '-';
    perm_str[8] = (permissions & 0001) ? 'x' : '-';
    
    perm_str[9] = '\0';
} 