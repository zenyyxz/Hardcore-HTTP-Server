#ifndef TYPES_H
#define TYPES_H

typedef unsigned long size_t;
typedef long ssize_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int64_t;
typedef unsigned long uint64_t;

#define NULL 0

// File Descriptor constants
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// Open flags
#define O_RDONLY 0

// Networking constants
#define AF_INET 2
#define SOCK_STREAM 1
#define INADDR_ANY 0
#define SOL_SOCKET 1
#define SO_REUSEADDR 2

// Byte swapping
#define htons(n) (((((uint16_t)(n)) & 0xFF00) >> 8) | ((((uint16_t)(n)) & 0x00FF) << 8))

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr {
    uint16_t sa_family;
    char sa_data[14];
};

typedef uint32_t socklen_t;

// Stat structure (simplified for x86_64)
struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_nlink;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t __pad0;
    uint64_t st_rdev;
    int64_t st_size;
    int64_t st_blksize;
    int64_t st_blocks;
    uint64_t st_atime;
    uint64_t st_atime_nsec;
    uint64_t st_mtime;
    uint64_t st_mtime_nsec;
    uint64_t st_ctime;
    uint64_t st_ctime_nsec;
    int64_t __unused[3];
};

#define DT_DIR 4
#define DT_REG 8

struct linux_dirent64 {
    uint64_t        d_ino;
    int64_t         d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[];
};

// Stat macros
#define S_ISDIR(m) (((m) & 0170000) == 0040000)
// ZIP format structures
struct zip_local_header {
    uint32_t signature;      // 0x04034b50
    uint16_t version;
    uint16_t flags;
    uint16_t compression;    // 0 = Store
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
} __attribute__((packed));

struct zip_central_header {
    uint32_t signature;      // 0x02014b50
    uint16_t version_made;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
    uint16_t comment_len;
    uint16_t disk_start;
    uint16_t attr_internal;
    uint32_t attr_external;
    uint32_t local_header_offset;
} __attribute__((packed));

struct zip_eocd {
    uint32_t signature;      // 0x06054b50
    uint16_t disk_num;
    uint16_t cd_disk_num;
    uint16_t cd_records_disk;
    uint16_t cd_records_total;
    uint32_t cd_size;
    uint32_t cd_offset;
    uint16_t comment_len;
} __attribute__((packed));

#endif
