#ifndef KLIB_H
#define KLIB_H

#include <stdarg.h>
#include "comm/types.h"

// 格式控制信息结构体
typedef struct
{
    int min_width; // 最小宽度
    char pad_char; // 填充字符
    int is_zero;   // 是否补零
} fmt_spec_t;

static inline uint32_t down2(uint32_t size, uint32_t bound)
{
    return size & ~(bound - 1);
}

static inline uint32_t up2(uint32_t size, uint32_t bound)
{
    return (size + bound - 1) & ~(bound - 1);
}

void kernel_strcpy(char *dest, const char *src);

void kernel_strncpy(char *dest, const char *src, int size);

int kernel_strncmp(const char *str1, const char *str2, int size);

int kernel_strlen(const char *str);

void kernel_memcpy(void *dest, void *src, int size);

void kernel_memset(void *dest, uint8_t val, int size);

int kernel_memcmp(const void *str1, const void *str2, int size);

void kernel_vsnprintf(char *buf, const char *fmt, va_list args);

void kernel_sprintf(char *buf, const char *fmt, ...);

int string_count(char **start);

char *get_file_name(char *name);



#ifndef RELEASE
#define ASSERT(expr) \
    if (!(expr))     \
    pannic(__FILE__, __LINE__, __func__, #expr)
void pannic(const char *file, int line, const char *func, const char *cond);

#else
#define ASSERT(expr) ((void)0)
#endif
#endif