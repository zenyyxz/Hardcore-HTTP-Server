#include "string_utils.h"
#include "syscalls.h"

extern "C" {
    size_t strlen(const char* s) {
        const char* p = s;
        while (*p) p++;
        return p - s;
    }

    void* memcpy(void* dest, const void* src, size_t n) {
        char* d = (char*)dest;
        const char* s = (const char*)src;
        while (n--) *d++ = *s++;
        return dest;
    }

    void* memset(void* s, int c, size_t n) {
        char* p = (char*)s;
        while (n--) *p++ = (char)c;
        return s;
    }

    int strcmp(const char* s1, const char* s2) {
        while (*s1 && (*s1 == *s2)) {
            s1++;
            s2++;
        }
        return *(unsigned char*)s1 - *(unsigned char*)s2;
    }

    int strncmp(const char* s1, const char* s2, size_t n) {
        if (!n) return 0;
        while (--n && *s1 && *s1 == *s2) {
            s1++;
            s2++;
        }
        return *(unsigned char*)s1 - *(unsigned char*)s2;
    }

    char* strchr(const char* s, int c) {
        while (*s != (char)c) {
            if (!*s) return 0;
            s++;
        }
        return (char*)s;
    }

    char* strncpy(char* dest, const char* src, size_t n) {
        size_t i;
        for (i = 0; i < n && src[i] != '\0'; i++) {
            dest[i] = src[i];
        }
        for (; i < n; i++) {
            dest[i] = '\0';
        }
        return dest;
    }

    char* itoa(int n, char* s, int base) {
        int i = 0;
        bool neg = false;
        
        if (n == 0) {
            s[i++] = '0';
            s[i] = '\0';
            return s;
        }
        
        if (n < 0 && base == 10) {
            neg = true;
            n = -n;
        }
        
        while (n != 0) {
            int rem = n % base;
            s[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
            n = n / base;
        }
        
        if (neg) s[i++] = '-';
        s[i] = '\0';

        // reverse it
        for (int j = 0; j < i / 2; j++) {
            char t = s[j];
            s[j] = s[i - j - 1];
            s[i - j - 1] = t;
        }
        return s;
    }
}

void print(const char* s) {
    sys_write(STDOUT_FILENO, s, strlen(s));
}

void print_int(int n) {
    char buf[12];
    itoa(n, buf, 10);
    print(buf);
}

void print_ip(uint32_t ip) {
    for (int i = 0; i < 4; i++) {
        char buf[4];
        itoa((ip >> (i * 8)) & 0xFF, buf, 10);
        print(buf);
        if (i < 3) print(".");
    }
}
