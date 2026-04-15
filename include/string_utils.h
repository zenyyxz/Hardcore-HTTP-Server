#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "types.h"

extern "C" {
    size_t strlen(const char* s);
    void* memcpy(void* dest, const void* src, size_t n);
    void* memset(void* s, int c, size_t n);
    int strcmp(const char* s1, const char* s2);
    int strncmp(const char* s1, const char* s2, size_t n);
    char* strchr(const char* s, int c);
    char* strncpy(char* dest, const char* src, size_t n);
    char* itoa(int n, char* s, int base);
    uint32_t crc32(const void* data, size_t n, uint32_t seed);
    uint32_t parse_ip(const char* s);
    void url_decode(char* str);
}

void print(const char* s);
void print_int(int n);
void print_ip(uint32_t ip);

#endif
