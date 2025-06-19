/**
 * Simple File System Core Header
 * fs.h
 * 
 * Defines core data structures and interfaces for a Unix-like file system
 * This implementation provides basic file system functionality including
 * superblock management, inode operations, directory handling, and file I/O.
 * 
 * Design Philosophy:
 * - Unix-like design with superblock, inodes, and directory entries
 * - Simple and educational implementation
 * - Clear separation of concerns between different components
 */

#ifndef _FS_H_
#define _FS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>

/*==============================================================================
 * FILESYSTEM CONSTANTS AND CONFIGURATION
 *============================================================================*/

#define FS_MAGIC_NUMBER     0x53465321      // "SFS!" - Simple File System magic
#define BLOCK_SIZE          1024            // Size of each data block in bytes
#define MAX_FILENAME_LEN    64              // Maximum length of a filename
#define MAX_PATH_LEN        256             // Maximum path length
#define DIRECT_BLOCKS       12              // Number of direct block pointers in inode
#define INDIRECT_BLOCKS     1               // Number of indirect block pointers
#define MAX_INODES          1024            // Maximum number of inodes
#define MAX_DATA_BLOCKS     4096            // Maximum number of data blocks
#define MAX_OPEN_FILES      64              // Maximum simultaneously open files
#define MAX_USERS           32              // Maximum number of users
#define ROOT_INODE_NUM      1               // Root directory inode number (0 is reserved)

/*==============================================================================
 * FILE SYSTEM TYPES AND ENUMS
 *============================================================================*/

/**
 * File types supported by the file system
 * Compatible with Unix file type conventions
 */
typedef enum {
    FS_FILE_TYPE_REGULAR    = 0x1,          // Regular file (S_IFREG equivalent)
    FS_FILE_TYPE_DIRECTORY  = 0x2,          // Directory (S_IFDIR equivalent)  
    FS_FILE_TYPE_SYMLINK    = 0x3,          // Symbolic link (S_IFLNK equivalent)
    FS_FILE_TYPE_SPECIAL    = 0x4           // Special file (device, pipe, etc.)
} fs_file_type_t;

/**
 * File permissions (Unix-style permission bits)
 * Can be combined using bitwise OR operations
 */
typedef enum {
    FS_PERM_OWNER_READ      = 0400,         // Owner read permission
    FS_PERM_OWNER_WRITE     = 0200,         // Owner write permission  
    FS_PERM_OWNER_EXEC      = 0100,         // Owner execute permission
    FS_PERM_GROUP_READ      = 0040,         // Group read permission
    FS_PERM_GROUP_WRITE     = 0020,         // Group write permission
    FS_PERM_GROUP_EXEC      = 0010,         // Group execute permission
    FS_PERM_OTHER_READ      = 0004,         // Others read permission
    FS_PERM_OTHER_WRITE     = 0002,         // Others write permission
    FS_PERM_OTHER_EXEC      = 0001,         // Others execute permission
    
    // Convenience combinations
    FS_PERM_OWNER_ALL       = 0700,         // Owner: rwx
    FS_PERM_GROUP_ALL       = 0070,         // Group: rwx  
    FS_PERM_OTHER_ALL       = 0007,         // Others: rwx
    FS_PERM_ALL_READ        = 0444,         // All: r--
    FS_PERM_ALL_WRITE       = 0222,         // All: -w-
    FS_PERM_ALL_EXEC        = 0111          // All: --x
} fs_permission_t;

/**
 * File system error codes
 * Provides detailed error information for all operations
 */
typedef enum {
    FS_SUCCESS              = 0,             // Operation successful
    FS_ERROR_INVALID_PARAM  = -1,           // Invalid parameter passed
    FS_ERROR_NO_MEMORY      = -2,           // Out of memory
    FS_ERROR_NO_SPACE       = -3,           // No space left on device
    FS_ERROR_FILE_NOT_FOUND = -4,           // File or directory not found
    FS_ERROR_FILE_EXISTS    = -5,           // File already exists
    FS_ERROR_NOT_DIRECTORY  = -6,           // Not a directory
    FS_ERROR_IS_DIRECTORY   = -7,           // Is a directory
    FS_ERROR_PERMISSION     = -8,           // Permission denied
    FS_ERROR_FILE_OPEN      = -9,           // File is currently open
    FS_ERROR_TOO_MANY_OPEN  = -10,          // Too many open files
    FS_ERROR_IO             = -11,          // I/O error
    FS_ERROR_CORRUPTED      = -12,          // File system corrupted
    FS_ERROR_NOT_MOUNTED    = -13,          // File system not mounted
    FS_ERROR_ALREADY_MOUNTED = -14          // File system already mounted
} fs_error_t;

/*==============================================================================
 * CORE DATA STRUCTURES
 *============================================================================*/

/**
 * Superblock Structure
 * 
 * The superblock contains metadata about the entire file system.
 * It's the first structure read when mounting the file system and
 * contains critical information needed for all file system operations.
 * 
 * Location: Typically stored at the beginning of the file system
 * Size: Should fit within one block for efficient I/O
 */
typedef struct {
    /* File system identification */
    uint32_t    magic_number;               // Magic number for file system identification
    uint32_t    version;                    // File system version number
    uint32_t    block_size;                 // Size of each data block (should be BLOCK_SIZE)
    
    /* Size and capacity information */
    uint32_t    total_blocks;               // Total number of blocks in file system
    uint32_t    total_inodes;               // Total number of inodes available
    uint32_t    free_blocks;                // Number of free data blocks
    uint32_t    free_inodes;                // Number of free inodes
    
    /* Layout information */
    uint32_t    inode_table_start;          // Starting block of inode table
    uint32_t    inode_table_blocks;         // Number of blocks in inode table
    uint32_t    data_blocks_start;          // Starting block of data area
    uint32_t    root_inode;                 // Inode number of root directory
    
    /* File system state and statistics */
    uint32_t    mount_count;                // Number of times file system has been mounted
    uint32_t    max_mount_count;            // Maximum mount count before check required
    time_t      created_time;               // File system creation timestamp
    time_t      last_mount_time;            // Last mount timestamp
    time_t      last_write_time;            // Last write operation timestamp
    time_t      last_check_time;            // Last file system check timestamp
    
    /* Reserved space for future use */
    uint32_t    reserved[16];               // Reserved for future features
    uint32_t    checksum;                   // Superblock checksum for integrity
} __attribute__((packed)) fs_superblock_t;

/**
 * Inode Structure  
 *
 * The inode (index node) is the core data structure representing a file
 * or directory in the file system. It contains all metadata about the file
 * except for the filename (which is stored in directory entries).
 *
 * Design follows Unix inode conventions with direct and indirect block pointers
 * for efficient storage of both small and large files.
 */
typedef struct {
    /* File identification and type */
    uint32_t    inode_number;               // Unique inode identifier
    uint16_t    file_type;                  // File type (regular, directory, symlink, etc.)
    uint16_t    permissions;                // File permissions (Unix-style: rwxrwxrwx)
    
    /* Ownership and access control */
    uint32_t    owner_uid;                  // User ID of file owner
    uint32_t    owner_gid;                  // Group ID of file owner
    uint32_t    link_count;                 // Number of hard links to this inode
    
    /* File size and block information */
    uint64_t    file_size;                  // File size in bytes
    uint32_t    block_count;                // Number of blocks allocated to this file
    
    /* Timestamps (Unix-style) */
    time_t      access_time;                // Last access time (atime)
    time_t      modify_time;                // Last modification time (mtime)  
    time_t      change_time;                // Last inode change time (ctime)
    time_t      create_time;                // Creation time (birth time)
    
    /* Block pointers for file data */
    uint32_t    direct_blocks[DIRECT_BLOCKS];   // Direct pointers to data blocks
    uint32_t    indirect_block;             // Pointer to indirect block
    uint32_t    double_indirect_block;      // Pointer to double indirect block (future use)
    uint32_t    triple_indirect_block;      // Pointer to triple indirect block (future use)
    
    /* Additional metadata */
    uint32_t    flags;                      // File flags (immutable, append-only, etc.)
    uint32_t    generation;                 // File version (for NFS)
    
    /* Reserved space */
    uint32_t    reserved[4];                // Reserved for future use
} __attribute__((packed)) fs_inode_t;

/**
 * Directory Entry Structure
 *
 * Directory entries map filenames to inode numbers. Multiple directory
 * entries can point to the same inode (hard links). The directory entry
 * is stored within the data blocks of directory inodes.
 *
 * Design considerations:
 * - Fixed-size entries for simplicity
 * - Includes validity flag for deleted entries
 * - Filename length limited for performance
 */
typedef struct {
    uint32_t    inode_number;               // Inode number this entry points to
    uint16_t    entry_length;               // Length of this directory entry
    uint8_t     filename_length;            // Length of filename
    uint8_t     file_type;                  // File type (for performance)
    char        filename[MAX_FILENAME_LEN]; // Null-terminated filename
    uint8_t     is_valid;                   // Entry validity flag (1=valid, 0=deleted)
    uint8_t     padding[3];                 // Padding for alignment
} __attribute__((packed)) fs_dir_entry_t;

/**
 * Block Bitmap Structure
 *
 * Manages allocation of data blocks and inodes using bitmaps.
 * Each bit represents one block/inode: 1=allocated, 0=free.
 */
typedef struct {
    uint8_t     *bitmap;                    // Bitmap data
    uint32_t    total_bits;                 // Total number of bits in bitmap
    uint32_t    free_count;                 // Number of free bits
    uint32_t    last_allocated;             // Last allocated bit (for optimization)
} fs_bitmap_t;

/**
 * File Control Block (Open File Handle)
 *
 * Represents an open file in the system. Contains current state
 * information for file I/O operations.
 */
typedef struct {
    uint32_t    inode_number;               // Inode number of open file
    uint32_t    flags;                      // Open flags (read, write, append, etc.)
    uint64_t    file_position;              // Current file position for I/O
    uint32_t    reference_count;            // Number of references to this handle
    time_t      open_time;                  // Time when file was opened
    uint32_t    owner_uid;                  // UID of process that opened file
} fs_file_handle_t;

/**
 * User Account Structure
 *
 * Represents a user account in the file system.
 * Simplified implementation for educational purposes.
 */
typedef struct {
    uint32_t    uid;                        // User identifier
    uint32_t    gid;                        // Primary group identifier
    char        username[32];               // Username (null-terminated)
    char        password_hash[64];          // Password hash (simplified)
    time_t      created_time;               // Account creation time
    uint8_t     is_active;                  // Account status (1=active, 0=disabled)
    uint8_t     padding[3];                 // Padding for alignment
} fs_user_t;

/**
 * File System State Structure
 *
 * Contains the complete runtime state of the file system.
 * This structure ties together all the components.
 */
typedef struct {
    /* Core file system components */
    fs_superblock_t     superblock;                     // File system metadata
    fs_bitmap_t         inode_bitmap;                   // Inode allocation bitmap
    fs_bitmap_t         block_bitmap;                   // Block allocation bitmap
    
    /* In-memory data structures */
    fs_inode_t          *inode_table;                   // In-memory inode table
    fs_file_handle_t    open_files[MAX_OPEN_FILES];     // Open file handles
    fs_user_t           users[MAX_USERS];               // User accounts
    
    /* Current state */
    uint32_t            current_user_uid;               // Currently logged in user
    uint32_t            current_directory_inode;        // Current working directory
    uint8_t             is_mounted;                     // Mount status
    uint8_t             is_dirty;                       // Needs synchronization
    uint8_t             read_only;                      // Read-only mode flag
    
    /* Statistics and debugging */
    uint32_t            total_reads;                    // Total read operations
    uint32_t            total_writes;                   // Total write operations
    uint32_t            cache_hits;                     // Cache hit count
    uint32_t            cache_misses;                   // Cache miss count
} fs_state_t;

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/* Global file system state */
extern fs_state_t g_filesystem_state;

/*------------------------------------------------------------------------------
 * File System Management Functions
 *----------------------------------------------------------------------------*/

/**
 * Initialize the file system
 * Creates initial data structures and prepares for mounting
 */
fs_error_t fs_init(void);

/**
 * Create a new file system
 * Formats the storage and creates initial superblock, root directory, etc.
 */
fs_error_t fs_create_filesystem(uint32_t total_blocks);

/**
 * Mount the file system
 * Loads superblock and prepares file system for operations
 */
fs_error_t fs_mount(void);

/**
 * Unmount the file system  
 * Synchronizes all pending writes and releases resources
 */
fs_error_t fs_unmount(void);

/**
 * Synchronize file system
 * Flushes all pending writes to storage
 */
fs_error_t fs_sync(void);

/**
 * Check file system integrity
 * Performs consistency checks and reports errors
 */
fs_error_t fs_check(void);

/*------------------------------------------------------------------------------
 * Inode Management Functions
 *----------------------------------------------------------------------------*/

/**
 * Allocate a new inode
 * Returns inode number of newly allocated inode, 0 on failure
 */
uint32_t fs_inode_alloc(fs_file_type_t file_type, uint16_t permissions, 
                        uint32_t owner_uid, uint32_t owner_gid);

/**
 * Free an inode
 * Releases inode and all associated data blocks
 */
fs_error_t fs_inode_free(uint32_t inode_number);

/**
 * Read inode from storage
 * Loads inode data into provided structure
 */
fs_error_t fs_inode_read(uint32_t inode_number, fs_inode_t *inode);

/**
 * Write inode to storage
 * Saves inode data to persistent storage
 */
fs_error_t fs_inode_write(uint32_t inode_number, const fs_inode_t *inode);

/**
 * Update inode timestamps
 * Updates access, modify, and/or change times
 */
fs_error_t fs_inode_touch(uint32_t inode_number, int update_atime, 
                          int update_mtime, int update_ctime);

/*------------------------------------------------------------------------------
 * Block Management Functions  
 *----------------------------------------------------------------------------*/

/**
 * Allocate a data block
 * Returns block number of newly allocated block, 0 on failure
 */
uint32_t fs_block_alloc(void);

/**
 * Free a data block
 * Marks block as available for reuse
 */
fs_error_t fs_block_free(uint32_t block_number);

/**
 * Read data block
 * Reads block data into provided buffer
 */
fs_error_t fs_block_read(uint32_t block_number, void *buffer);

/**
 * Write data block
 * Writes buffer data to specified block
 */
fs_error_t fs_block_write(uint32_t block_number, const void *buffer);

/*------------------------------------------------------------------------------
 * File Operations
 *----------------------------------------------------------------------------*/

/**
 * Create a new file
 * Creates file with specified name, type, and permissions in current directory
 */
fs_error_t fs_file_create(const char *filename, fs_file_type_t file_type, 
                          uint16_t permissions);

/**
 * Open a file
 * Opens file for I/O operations, returns file handle ID
 */
fs_error_t fs_file_open(const char *filename, uint32_t flags, uint32_t *handle_id);

/**
 * Close a file
 * Closes file handle and flushes any pending writes
 */
fs_error_t fs_file_close(uint32_t handle_id);

/**
 * Read from file
 * Reads data from file at current position
 */
fs_error_t fs_file_read(uint32_t handle_id, void *buffer, size_t size, 
                        size_t *bytes_read);

/**
 * Write to file
 * Writes data to file at current position
 */
fs_error_t fs_file_write(uint32_t handle_id, const void *buffer, size_t size,
                         size_t *bytes_written);

/**
 * Seek in file
 * Changes current file position
 */
fs_error_t fs_file_seek(uint32_t handle_id, int64_t offset, int whence);

/**
 * Get file position
 * Returns current file position
 */
fs_error_t fs_file_tell(uint32_t handle_id, uint64_t *position);

/**
 * Truncate file
 * Changes file size to specified length
 */
fs_error_t fs_file_truncate(uint32_t handle_id, uint64_t new_size);

/**
 * Delete file
 * Removes file from directory and frees its resources
 */
fs_error_t fs_file_delete(const char *filename);

/*------------------------------------------------------------------------------
 * Directory Operations
 *----------------------------------------------------------------------------*/

/**
 * Create directory
 * Creates new directory with specified name and permissions
 */
fs_error_t fs_dir_create(const char *dirname, uint16_t permissions);

/**
 * Open directory for reading
 * Prepares directory for listing operations
 */
fs_error_t fs_dir_open(const char *dirname, uint32_t *handle_id);

/**
 * Read directory entry
 * Reads next directory entry from open directory
 */
fs_error_t fs_dir_read(uint32_t handle_id, fs_dir_entry_t *entry);

/**
 * Close directory
 * Closes directory handle
 */
fs_error_t fs_dir_close(uint32_t handle_id);

/**
 * Remove directory
 * Removes empty directory
 */
fs_error_t fs_dir_remove(const char *dirname);

/**
 * Change current directory
 * Changes current working directory
 */
fs_error_t fs_dir_change(const char *dirname);

/**
 * Get current directory
 * Returns current working directory path
 */
fs_error_t fs_dir_getcwd(char *buffer, size_t size);

/*------------------------------------------------------------------------------
 * File System Information and Statistics
 *----------------------------------------------------------------------------*/

/**
 * Get file/directory information
 * Returns detailed information about specified file
 */
fs_error_t fs_stat(const char *filename, fs_inode_t *stat_info);

/**
 * Get file system statistics
 * Returns file system usage and performance statistics
 */
fs_error_t fs_statfs(fs_superblock_t *stat_info);

/**
 * List directory contents
 * Lists all files in specified directory
 */
fs_error_t fs_list_directory(const char *dirname);

/*------------------------------------------------------------------------------
 * User Management Functions
 *----------------------------------------------------------------------------*/

/**
 * Create user account
 * Adds new user to the system
 */
fs_error_t fs_user_create(const char *username, const char *password, 
                          uint32_t uid, uint32_t gid);

/**
 * Authenticate user
 * Verifies user credentials and sets current user
 */
fs_error_t fs_user_login(const char *username, const char *password);

/**
 * Logout current user
 * Clears current user context
 */
fs_error_t fs_user_logout(void);

/**
 * Check file permissions
 * Verifies if current user has specified permissions for file
 */
int fs_permission_check(uint32_t inode_number, fs_permission_t required_perms);

/*------------------------------------------------------------------------------
 * Utility and Helper Functions
 *----------------------------------------------------------------------------*/

/**
 * Convert error code to string
 * Returns human-readable error message
 */
const char* fs_error_to_string(fs_error_t error);

/**
 * Print file system status
 * Displays current file system state and statistics
 */
void fs_print_status(void);

/**
 * Convert permissions to string
 * Formats permissions as Unix-style string (e.g., "rwxr-xr-x")
 */
void fs_permissions_to_string(uint16_t permissions, char *buffer);

/**
 * Get current timestamp
 * Returns current time for timestamping operations
 */
time_t fs_current_time(void);

/**
 * Calculate checksum
 * Computes checksum for data integrity verification
 */
uint32_t fs_calculate_checksum(const void *data, size_t size);

/*==============================================================================
 * MACRO DEFINITIONS AND INLINE FUNCTIONS
 *============================================================================*/

/* Convenience macros for permission checking */
#define FS_IS_READABLE(perms, uid, gid, file_uid, file_gid) \
    fs_check_permission_internal(perms, uid, gid, file_uid, file_gid, FS_PERM_OWNER_READ)

#define FS_IS_WRITABLE(perms, uid, gid, file_uid, file_gid) \
    fs_check_permission_internal(perms, uid, gid, file_uid, file_gid, FS_PERM_OWNER_WRITE)

#define FS_IS_EXECUTABLE(perms, uid, gid, file_uid, file_gid) \
    fs_check_permission_internal(perms, uid, gid, file_uid, file_gid, FS_PERM_OWNER_EXEC)

/* File type checking macros */
#define FS_IS_REGULAR_FILE(inode)   ((inode)->file_type == FS_FILE_TYPE_REGULAR)
#define FS_IS_DIRECTORY(inode)      ((inode)->file_type == FS_FILE_TYPE_DIRECTORY)
#define FS_IS_SYMLINK(inode)        ((inode)->file_type == FS_FILE_TYPE_SYMLINK)

/* Block and inode number validation */
#define FS_VALID_INODE_NUMBER(num)  ((num) > 0 && (num) <= MAX_INODES)
#define FS_VALID_BLOCK_NUMBER(num)  ((num) > 0 && (num) <= MAX_DATA_BLOCKS)

/* Internal helper function for permission checking */
static inline int fs_check_permission_internal(uint16_t file_perms, uint32_t user_uid, 
                                              uint32_t user_gid, uint32_t file_uid, 
                                              uint32_t file_gid, fs_permission_t required) {
    /* Root user has all permissions */
    if (user_uid == 0) return 1;
    
    /* Check owner permissions */
    if (user_uid == file_uid) {
        return (file_perms & required) != 0;
    }
    
    /* Check group permissions */
    if (user_gid == file_gid) {
        return (file_perms & (required >> 3)) != 0;
    }
    
    /* Check other permissions */
    return (file_perms & (required >> 6)) != 0;
}

#endif /* _FS_H_ */ 