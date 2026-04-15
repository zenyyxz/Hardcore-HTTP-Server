#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"

static inline long syscall0(long n) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(n) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall1(long n, long a1) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall2(long n, long a1, long a2) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    long ret;
    register long r10 asm("r10") = a4;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    register long r10 asm("r10") = a4;
    register long r8 asm("r8") = a5;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 asm("r10") = a4;
    register long r8 asm("r8") = a5;
    register long r9 asm("r9") = a6;
    asm volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return ret;
}

// Syscall Numbers for x86_64
#define SYS_read 0
#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_fstat 5
#define SYS_socket 41
#define SYS_accept 43
#define SYS_bind 49
#define SYS_listen 50
#define SYS_sendfile 40
#define SYS_exit 60

static inline ssize_t sys_sendfile(int out_fd, int in_fd, off_t* offset, size_t count) {
    return (ssize_t)syscall4(SYS_sendfile, out_fd, in_fd, (long)offset, count);
}

#define SYS_setsockopt 54

static inline int sys_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    return (int)syscall5(SYS_setsockopt, sockfd, level, optname, (long)optval, optlen);
}

#define SYS_getdents64 217

static inline int sys_getdents64(int fd, struct linux_dirent64* dirp, unsigned int count) {
    return (int)syscall3(SYS_getdents64, fd, (long)dirp, count);
}

static inline void sys_exit(int code) {
    syscall1(SYS_exit, code);
}

static inline ssize_t sys_write(int fd, const void* buf, size_t count) {
    return (ssize_t)syscall3(SYS_write, fd, (long)buf, count);
}

static inline ssize_t sys_read(int fd, void* buf, size_t count) {
    return (ssize_t)syscall3(SYS_read, fd, (long)buf, count);
}

static inline int sys_open(const char* pathname, int flags) {
    return (int)syscall2(SYS_open, (long)pathname, flags);
}

static inline int sys_close(int fd) {
    return (int)syscall1(SYS_close, fd);
}

static inline int sys_fstat(int fd, struct stat* statbuf) {
    return (int)syscall2(SYS_fstat, fd, (long)statbuf);
}

static inline int sys_socket(int domain, int type, int protocol) {
    return (int)syscall3(SYS_socket, domain, type, protocol);
}

static inline int sys_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return (int)syscall3(SYS_bind, sockfd, (long)addr, addrlen);
}

static inline int sys_listen(int sockfd, int backlog) {
    return (int)syscall2(SYS_listen, sockfd, backlog);
}

static inline int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return (int)syscall3(SYS_accept, sockfd, (long)addr, (long)addrlen);
}

#endif
