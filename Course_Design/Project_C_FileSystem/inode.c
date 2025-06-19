/**
 * inode管理模块
 * inode.c
 * 
 * 处理inode的分配、释放和管理
 */

#include "fs.h"

/* 初始化inode管理模块 */
fs_error_t inode_init(void) {
    // 清空inode表
    memset(g_fs.inodes, 0, sizeof(g_fs.inodes));
    
    // 初始化inode位图
    g_fs.inode_bitmap.size = (MAX_INODES + 7) / 8;  // 向上取整
    g_fs.inode_bitmap.bitmap = calloc(g_fs.inode_bitmap.size, 1);
    if (!g_fs.inode_bitmap.bitmap) {
        return FS_ERROR_NO_SPACE;
    }
    g_fs.inode_bitmap.free_count = MAX_INODES - 1;  // 0号inode保留
    
    // 标记0号inode为已使用（保留）
    g_fs.inode_bitmap.bitmap[0] |= 0x01;
    
    // 创建根目录inode (inode 1)
    uint32_t root_inode = inode_alloc();
    if (root_inode != 1) {
        free(g_fs.inode_bitmap.bitmap);
        return FS_ERROR_IO;
    }
    
    inode_t *root = inode_get(root_inode);
    root->type = FILE_TYPE_DIRECTORY;
    root->size = 0;
    root->owner_uid = 1;  // root用户
    root->owner_gid = 0;  // root组
    root->permissions = 0755;  // rwxr-xr-x
    root->created_time = fs_current_time();
    root->modified_time = root->created_time;
    root->accessed_time = root->created_time;
    root->link_count = 2;  // . 和 ..
    root->block_count = 0;
    
    // 初始化块索引
    for (int i = 0; i < 12; i++) {
        root->direct_blocks[i] = 0;
    }
    root->indirect_block = 0;
    
    // 设置根目录为当前目录
    g_fs.current_directory = root_inode;
    g_fs.superblock.root_inode = root_inode;
    
    printf("inode管理模块初始化完成，根目录已创建 (inode %d)\n", root_inode);
    return FS_SUCCESS;
}

/* 分配新的inode */
uint32_t inode_alloc(void) {
    if (g_fs.inode_bitmap.free_count == 0) {
        return 0;  // 没有可用的inode
    }
    
    // 查找第一个空闲的inode
    for (uint32_t i = 1; i < MAX_INODES; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_index = i % 8;
        
        if (!(g_fs.inode_bitmap.bitmap[byte_index] & (1 << bit_index))) {
            // 找到空闲inode，标记为已使用
            g_fs.inode_bitmap.bitmap[byte_index] |= (1 << bit_index);
            g_fs.inode_bitmap.free_count--;
            g_fs.superblock.free_inodes--;
            
            // 初始化inode
            inode_t *inode = &g_fs.inodes[i];
            memset(inode, 0, sizeof(inode_t));
            inode->inode_id = i;
            inode->is_used = 1;
            
            return i;
        }
    }
    
    return 0;  // 没有找到空闲inode
}

/* 释放inode */
fs_error_t inode_free(uint32_t inode_id) {
    if (inode_id == 0 || inode_id >= MAX_INODES) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    if (inode_id == 1) {
        return FS_ERROR_PERMISSION;  // 不能删除根目录
    }
    
    uint32_t byte_index = inode_id / 8;
    uint32_t bit_index = inode_id % 8;
    
    // 检查inode是否已被使用
    if (!(g_fs.inode_bitmap.bitmap[byte_index] & (1 << bit_index))) {
        return FS_ERROR_INVALID_PARAM;  // inode未被使用
    }
    
    // 获取inode信息
    inode_t *inode = inode_get(inode_id);
    if (!inode || !inode->is_used) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 释放数据块（简化实现，这里只是清除索引）
    for (int i = 0; i < 12; i++) {
        inode->direct_blocks[i] = 0;
    }
    inode->indirect_block = 0;
    
    // 清空inode
    memset(inode, 0, sizeof(inode_t));
    
    // 标记inode为空闲
    g_fs.inode_bitmap.bitmap[byte_index] &= ~(1 << bit_index);
    g_fs.inode_bitmap.free_count++;
    g_fs.superblock.free_inodes++;
    
    return FS_SUCCESS;
}

/* 获取inode */
inode_t* inode_get(uint32_t inode_id) {
    if (inode_id == 0 || inode_id >= MAX_INODES) {
        return NULL;
    }
    
    if (!g_fs.inodes[inode_id].is_used) {
        return NULL;
    }
    
    return &g_fs.inodes[inode_id];
}

/* 更新inode */
fs_error_t inode_update(uint32_t inode_id, inode_t *inode) {
    if (!inode || inode_id == 0 || inode_id >= MAX_INODES) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    if (!g_fs.inodes[inode_id].is_used) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 复制inode数据（保留inode_id和is_used字段）
    uint32_t saved_id = g_fs.inodes[inode_id].inode_id;
    int saved_used = g_fs.inodes[inode_id].is_used;
    
    g_fs.inodes[inode_id] = *inode;
    g_fs.inodes[inode_id].inode_id = saved_id;
    g_fs.inodes[inode_id].is_used = saved_used;
    
    // 更新修改时间
    g_fs.inodes[inode_id].modified_time = fs_current_time();
    
    return FS_SUCCESS;
}

/* 检查inode是否为目录 */
int inode_is_directory(uint32_t inode_id) {
    inode_t *inode = inode_get(inode_id);
    if (!inode) {
        return 0;
    }
    return inode->type == FILE_TYPE_DIRECTORY;
}

/* 检查inode是否为普通文件 */
int inode_is_regular_file(uint32_t inode_id) {
    inode_t *inode = inode_get(inode_id);
    if (!inode) {
        return 0;
    }
    return inode->type == FILE_TYPE_REGULAR;
}

/* 获取inode统计信息 */
void inode_get_stats(uint32_t *total, uint32_t *used, uint32_t *free) {
    if (total) *total = MAX_INODES - 1;  // 不包括保留的0号inode
    if (free) *free = g_fs.inode_bitmap.free_count;
    if (used) *used = (MAX_INODES - 1) - g_fs.inode_bitmap.free_count;
}

/* 列出所有使用中的inode */
void inode_list_all(void) {
    printf("inode使用情况:\n");
    printf("%-8s %-10s %-8s %-10s %-6s %-6s %-20s\n", 
           "inode", "类型", "大小", "所有者", "权限", "链接", "修改时间");
    printf("------------------------------------------------------------------------\n");
    
    for (uint32_t i = 1; i < MAX_INODES; i++) {
        if (g_fs.inodes[i].is_used) {
            inode_t *inode = &g_fs.inodes[i];
            char time_str[32];
            char type_str[16];
            char perm_str[12];
            
            // 格式化时间
            struct tm *tm_info = localtime(&inode->modified_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            // 格式化类型
            switch (inode->type) {
                case FILE_TYPE_REGULAR:
                    strcpy(type_str, "普通文件");
                    break;
                case FILE_TYPE_DIRECTORY:
                    strcpy(type_str, "目录");
                    break;
                case FILE_TYPE_SYMLINK:
                    strcpy(type_str, "符号链接");
                    break;
                default:
                    strcpy(type_str, "未知");
                    break;
            }
            
            // 格式化权限
            snprintf(perm_str, sizeof(perm_str), "%o", inode->permissions);
            
            printf("%-8d %-10s %-8d %d:%-7d %-6s %-6d %-20s\n",
                   inode->inode_id,
                   type_str,
                   inode->size,
                   inode->owner_uid,
                   inode->owner_gid,
                   perm_str,
                   inode->link_count,
                   time_str);
        }
    }
    
    uint32_t total, used, free;
    inode_get_stats(&total, &used, &free);
    printf("\ninode统计: 总数=%d, 已用=%d, 空闲=%d\n", total, used, free);
}

/* 设置inode的权限 */
fs_error_t inode_set_permissions(uint32_t inode_id, uint16_t permissions) {
    inode_t *inode = inode_get(inode_id);
    if (!inode) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 检查当前用户是否有权限修改
    if (g_fs.current_user != 1 && g_fs.current_user != inode->owner_uid) {
        return FS_ERROR_PERMISSION;
    }
    
    inode->permissions = permissions;
    inode->modified_time = fs_current_time();
    
    return FS_SUCCESS;
}

/* 更改inode的所有者 */
fs_error_t inode_change_owner(uint32_t inode_id, uint32_t new_uid, uint32_t new_gid) {
    inode_t *inode = inode_get(inode_id);
    if (!inode) {
        return FS_ERROR_INVALID_PARAM;
    }
    
    // 只有root用户可以更改所有者
    if (g_fs.current_user != 1) {
        return FS_ERROR_PERMISSION;
    }
    
    // 验证新的UID是否存在
    if (new_uid >= MAX_USERS || !g_fs.users[new_uid].is_active) {
        return FS_ERROR_USER_NOT_FOUND;
    }
    
    inode->owner_uid = new_uid;
    inode->owner_gid = new_gid;
    inode->modified_time = fs_current_time();
    
    return FS_SUCCESS;
} 