/**
 * User Management System Header
 * user_manager.h
 * 
 * 实现简单的用户管理系统，包括用户创建、登录、登出和权限检查
 */

#ifndef _USER_MANAGER_H_
#define _USER_MANAGER_H_

#include "fs.h"
#include <stdint.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/

#define USER_MANAGER_ROOT_UID       0           // 超级用户UID
#define USER_MANAGER_ROOT_GID       0           // 超级用户GID
#define USER_MANAGER_DEFAULT_UID    1000        // 普通用户起始UID
#define USER_MANAGER_DEFAULT_GID    1000        // 普通用户起始GID
#define USER_MANAGER_ANONYMOUS_UID  65534       // 匿名用户UID

// 用户操作结果码
typedef enum {
    USER_SUCCESS            = 0,        // 操作成功
    USER_ERROR_INVALID_PARAM = -1,      // 参数无效
    USER_ERROR_USER_EXISTS  = -2,       // 用户已存在
    USER_ERROR_USER_NOT_FOUND = -3,     // 用户不存在
    USER_ERROR_WRONG_PASSWORD = -4,     // 密码错误
    USER_ERROR_NO_SPACE     = -5,       // 用户表已满
    USER_ERROR_PERMISSION   = -6,       // 权限不足
    USER_ERROR_NOT_LOGGED_IN = -7       // 未登录
} user_error_t;

/*==============================================================================
 * 用户管理函数声明
 *============================================================================*/

/**
 * 初始化用户管理系统
 * 创建默认的root用户和匿名用户
 * 
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_init(void);

/**
 * 创建新用户
 * 
 * @param username 用户名
 * @param password 密码（明文，内部会计算哈希）
 * @param uid 用户ID（0表示自动分配）
 * @param gid 组ID（0表示使用默认组）
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_create_user(const char* username, const char* password, 
                                     uint32_t uid, uint32_t gid);

/**
 * 用户登录
 * 
 * @param username 用户名
 * @param password 密码
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_login(const char* username, const char* password);

/**
 * 用户登出
 * 
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_logout(void);

/**
 * 获取当前登录用户信息
 * 
 * @param user_info 输出用户信息
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_get_current_user(fs_user_t* user_info);

/**
 * 根据UID获取用户信息
 * 
 * @param uid 用户ID
 * @param user_info 输出用户信息
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_get_user_by_uid(uint32_t uid, fs_user_t* user_info);

/**
 * 根据用户名获取用户信息
 * 
 * @param username 用户名
 * @param user_info 输出用户信息
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_get_user_by_name(const char* username, fs_user_t* user_info);

/**
 * 检查当前用户是否有指定权限
 * 
 * @param inode 文件inode
 * @param required_perm 所需权限
 * @return 1表示有权限，0表示无权限
 */
int user_manager_check_permission(const fs_inode_t* inode, fs_permission_t required_perm);

/**
 * 检查用户是否为超级用户
 * 
 * @param uid 用户ID
 * @return 1表示是超级用户，0表示不是
 */
int user_manager_is_root(uint32_t uid);

/**
 * 获取当前用户UID
 * 
 * @return 当前用户UID
 */
uint32_t user_manager_get_current_uid(void);

/**
 * 获取当前用户GID
 * 
 * @return 当前用户GID
 */
uint32_t user_manager_get_current_gid(void);

/**
 * 设置文件所有者（仅超级用户可用）
 * 
 * @param inode_number inode号
 * @param new_uid 新的所有者UID
 * @param new_gid 新的所有者GID
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_chown(uint32_t inode_number, uint32_t new_uid, uint32_t new_gid);

/**
 * 修改文件权限
 * 
 * @param inode_number inode号
 * @param new_permissions 新的权限位
 * @return USER_SUCCESS 或错误码
 */
user_error_t user_manager_chmod(uint32_t inode_number, uint16_t new_permissions);

/**
 * 列出所有用户
 */
void user_manager_list_users(void);

/**
 * 将权限转换为字符串（如 "rwxr-xr--"）
 * 
 * @param permissions 权限位
 * @param buffer 输出缓冲区（至少10字节）
 */
void user_manager_permissions_to_string(uint16_t permissions, char* buffer);

/**
 * 计算简单密码哈希（仅用于演示）
 * 
 * @param password 明文密码
 * @param hash_buffer 输出哈希缓冲区（64字节）
 */
void user_manager_hash_password(const char* password, char* hash_buffer);

/*==============================================================================
 * 内部辅助函数声明（供其他模块使用）
 *============================================================================*/

/**
 * 检查特定用户对文件的权限
 * 
 * @param user_uid 用户UID
 * @param user_gid 用户GID
 * @param file_uid 文件所有者UID
 * @param file_gid 文件所有者GID
 * @param file_permissions 文件权限位
 * @param required_perm 所需权限
 * @return 1表示有权限，0表示无权限
 */
int user_manager_check_permission_detailed(uint32_t user_uid, uint32_t user_gid,
                                          uint32_t file_uid, uint32_t file_gid,
                                          uint16_t file_permissions, 
                                          fs_permission_t required_perm);

#endif /* _USER_MANAGER_H_ */ 