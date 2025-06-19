/**
 * Disk Simulator Header
 * disk_simulator.h
 * 
 * Simulates a block-based disk using a single host OS file.
 * Provides basic disk operations like read/write blocks with proper error handling.
 * 
 * This module abstracts the underlying file system and provides a clean
 * block-device interface for the file system implementation.
 */

#ifndef _DISK_SIMULATOR_H_
#define _DISK_SIMULATOR_H_

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <sys/stat.h>

/*==============================================================================
 * DISK SIMULATOR CONSTANTS
 *============================================================================*/

#define DISK_BLOCK_SIZE         1024        // Size of each disk block in bytes
#define DISK_MAX_FILENAME_LEN   256         // Maximum length of disk filename
#define DISK_MAGIC_HEADER       0x44534B21  // "DSK!" - Disk magic number
#define DISK_VERSION            1           // Disk format version

/*==============================================================================
 * ERROR CODES
 *============================================================================*/

/**
 * Disk operation error codes
 * Negative values indicate errors, 0 indicates success
 */
typedef enum {
    DISK_SUCCESS            = 0,    // Operation successful
    DISK_ERROR_INVALID_PARAM = -1,  // Invalid parameter
    DISK_ERROR_FILE_OPEN    = -2,   // Failed to open disk file
    DISK_ERROR_FILE_CREATE  = -3,   // Failed to create disk file
    DISK_ERROR_FILE_READ    = -4,   // Failed to read from disk file
    DISK_ERROR_FILE_WRITE   = -5,   // Failed to write to disk file
    DISK_ERROR_FILE_SEEK    = -6,   // Failed to seek in disk file
    DISK_ERROR_BLOCK_RANGE  = -7,   // Block number out of range
    DISK_ERROR_NOT_INIT     = -8,   // Disk not initialized
    DISK_ERROR_ALREADY_INIT = -9,   // Disk already initialized
    DISK_ERROR_DISK_FULL    = -10,  // Disk is full
    DISK_ERROR_IO           = -11,  // General I/O error
    DISK_ERROR_CORRUPTED    = -12   // Disk data corrupted
} disk_error_t;

/*==============================================================================
 * DATA STRUCTURES
 *============================================================================*/

/**
 * Disk Header Structure
 * 
 * Stored at the beginning of the disk file to identify and validate
 * the disk format. Contains metadata about the disk layout.
 */
typedef struct {
    uint32_t    magic_number;       // Magic number for identification
    uint32_t    version;            // Disk format version
    uint32_t    block_size;         // Size of each block
    uint32_t    total_blocks;       // Total number of blocks on disk
    uint32_t    disk_size;          // Total disk size in bytes
    time_t      created_time;       // Disk creation timestamp
    time_t      last_access_time;   // Last access timestamp
    uint32_t    checksum;           // Header checksum for integrity
    uint8_t     reserved[32];       // Reserved space for future use
} __attribute__((packed)) disk_header_t;

/**
 * Disk Statistics Structure
 * 
 * Maintains runtime statistics about disk operations
 * for performance monitoring and debugging.
 */
typedef struct {
    uint64_t    total_reads;        // Total number of read operations
    uint64_t    total_writes;       // Total number of write operations
    uint64_t    bytes_read;         // Total bytes read
    uint64_t    bytes_written;      // Total bytes written
    uint64_t    read_errors;        // Number of read errors
    uint64_t    write_errors;       // Number of write errors
    time_t      last_operation_time;// Time of last operation
    double      avg_read_time;      // Average read time (seconds)
    double      avg_write_time;     // Average write time (seconds)
} disk_stats_t;

/**
 * Disk State Structure
 * 
 * Maintains the current state of the disk simulator.
 * Contains all information needed for disk operations.
 */
typedef struct {
    /* File handling */
    int         fd;                 // File descriptor for disk file
    char        filename[DISK_MAX_FILENAME_LEN]; // Path to disk file
    
    /* Disk configuration */
    uint32_t    total_blocks;       // Total number of blocks
    uint32_t    block_size;         // Size of each block (should be DISK_BLOCK_SIZE)
    uint64_t    disk_size;          // Total disk size in bytes
    
    /* State flags */
    uint8_t     is_initialized;     // Whether disk is initialized
    uint8_t     is_read_only;       // Whether disk is read-only
    uint8_t     is_dirty;           // Whether disk has pending writes
    
    /* Statistics */
    disk_stats_t stats;             // Operation statistics
    
    /* Synchronization and caching */
    uint8_t     auto_sync;          // Auto-sync after each write
    time_t      last_sync_time;     // Last synchronization time
} disk_state_t;

/*==============================================================================
 * GLOBAL VARIABLES
 *============================================================================*/

/* Global disk state - external declaration */
extern disk_state_t g_disk_state;

/*==============================================================================
 * CORE DISK OPERATIONS
 *============================================================================*/

/**
 * Initialize the disk simulator
 * 
 * Creates or opens a file representing the disk. If the file doesn't exist,
 * it will be created with the specified size. If it exists, it will be
 * validated and opened for use.
 * 
 * @param filename Path to the disk file
 * @param disk_size Size of the disk in bytes (must be multiple of block size)
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_init(const char* filename, int disk_size);

/**
 * Write a block of data to the disk
 * 
 * Writes exactly one block (DISK_BLOCK_SIZE bytes) of data to the specified
 * block number. The block number is 0-based.
 * 
 * @param block_num Block number to write to (0-based)
 * @param data Pointer to data buffer (must be at least DISK_BLOCK_SIZE bytes)
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_write_block(int block_num, const char* data);

/**
 * Read a block of data from the disk
 * 
 * Reads exactly one block (DISK_BLOCK_SIZE bytes) of data from the specified
 * block number into the provided buffer.
 * 
 * @param block_num Block number to read from (0-based)
 * @param buffer Buffer to store read data (must be at least DISK_BLOCK_SIZE bytes)
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_read_block(int block_num, char* buffer);

/*==============================================================================
 * EXTENDED DISK OPERATIONS
 *============================================================================*/

/**
 * Close and cleanup the disk simulator
 * 
 * Synchronizes any pending writes, closes the disk file, and resets
 * the disk state. Should be called before program termination.
 * 
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_close(void);

/**
 * Synchronize disk writes
 * 
 * Forces all pending writes to be flushed to the underlying storage.
 * Useful for ensuring data persistence at critical points.
 * 
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_sync(void);

/**
 * Get disk information
 * 
 * Retrieves current disk configuration and status information.
 * 
 * @param total_blocks Pointer to store total block count (can be NULL)
 * @param block_size Pointer to store block size (can be NULL)
 * @param disk_size Pointer to store total disk size (can be NULL)
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_get_info(uint32_t* total_blocks, uint32_t* block_size, uint64_t* disk_size);

/**
 * Get disk statistics
 * 
 * Retrieves current disk operation statistics for monitoring
 * and performance analysis.
 * 
 * @param stats Pointer to disk_stats_t structure to fill
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_get_stats(disk_stats_t* stats);

/**
 * Reset disk statistics
 * 
 * Resets all disk operation statistics to zero.
 * 
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_reset_stats(void);

/*==============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

/**
 * Convert disk error code to string
 * 
 * Returns a human-readable description of the disk error code.
 * 
 * @param error Disk error code
 * @return Pointer to error description string
 */
const char* disk_error_to_string(disk_error_t error);

/**
 * Check if disk is initialized
 * 
 * @return 1 if disk is initialized, 0 otherwise
 */
int disk_is_initialized(void);

/**
 * Get current disk block count
 * 
 * @return Number of blocks on disk, or 0 if not initialized
 */
uint32_t disk_get_block_count(void);

/**
 * Validate block number
 * 
 * Checks if the given block number is valid for the current disk.
 * 
 * @param block_num Block number to validate
 * @return 1 if valid, 0 if invalid
 */
int disk_is_valid_block(int block_num);

/**
 * Format disk with pattern
 * 
 * Fills the entire disk with a specified byte pattern.
 * Useful for initialization and testing.
 * 
 * @param pattern Byte pattern to fill disk with
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_format(uint8_t pattern);

/**
 * Print disk status
 * 
 * Prints current disk configuration and statistics to stdout.
 * Useful for debugging and monitoring.
 */
void disk_print_status(void);

/*==============================================================================
 * BLOCK I/O CONVENIENCE FUNCTIONS
 *============================================================================*/

/**
 * Write multiple consecutive blocks
 * 
 * Writes multiple blocks starting from the specified block number.
 * 
 * @param start_block Starting block number
 * @param block_count Number of blocks to write
 * @param data Data buffer (must be at least block_count * DISK_BLOCK_SIZE bytes)
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_write_blocks(int start_block, int block_count, const char* data);

/**
 * Read multiple consecutive blocks
 * 
 * Reads multiple blocks starting from the specified block number.
 * 
 * @param start_block Starting block number
 * @param block_count Number of blocks to read
 * @param buffer Buffer to store data (must be at least block_count * DISK_BLOCK_SIZE bytes)
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_read_blocks(int start_block, int block_count, char* buffer);

/**
 * Zero out a block
 * 
 * Fills the specified block with zeros.
 * 
 * @param block_num Block number to zero out
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_zero_block(int block_num);

/**
 * Copy block data
 * 
 * Copies data from one block to another on the same disk.
 * 
 * @param src_block Source block number
 * @param dst_block Destination block number
 * @return DISK_SUCCESS on success, negative error code on failure
 */
int disk_copy_block(int src_block, int dst_block);

/*==============================================================================
 * MACROS AND INLINE FUNCTIONS
 *============================================================================*/

/* Block size validation */
#define DISK_IS_BLOCK_ALIGNED(size) ((size) % DISK_BLOCK_SIZE == 0)

/* Block number to byte offset conversion */
#define DISK_BLOCK_TO_OFFSET(block_num) \
    (sizeof(disk_header_t) + ((uint64_t)(block_num) * DISK_BLOCK_SIZE))

/* Byte offset to block number conversion */
#define DISK_OFFSET_TO_BLOCK(offset) \
    (((offset) - sizeof(disk_header_t)) / DISK_BLOCK_SIZE)

/* Calculate number of blocks needed for given size */
#define DISK_SIZE_TO_BLOCKS(size) \
    (((size) + DISK_BLOCK_SIZE - 1) / DISK_BLOCK_SIZE)

/* Calculate total file size including header */
#define DISK_TOTAL_FILE_SIZE(blocks) \
    (sizeof(disk_header_t) + ((uint64_t)(blocks) * DISK_BLOCK_SIZE))

/**
 * Fast block bounds checking (inline for performance)
 */
static inline int disk_check_block_bounds(int block_num) {
    return (block_num >= 0 && 
            g_disk_state.is_initialized && 
            (uint32_t)block_num < g_disk_state.total_blocks);
}

#endif /* _DISK_SIMULATOR_H_ */ 