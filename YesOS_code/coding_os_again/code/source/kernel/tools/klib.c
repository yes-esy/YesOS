/**
 * @FilePath     : /code/source/kernel/tools/klib.c
 * @Description  :  工具库函数实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 15:18:42
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "tools/klib.h"
#include "comm/types.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"

/**
 * @brief        : 计算指针数组中的参数个数
 * @param         {char **} start: 起始地址
 * @return        {int} : 参数数量
 **/
int strings_count(char **start)
{
    int count = 0;
    if (start)
    {
        while (*start++)
        {
            count++;
        }
    }
    return count;
}
/**
 * @brief        : 从文件路径中获取文件名
 * @param         {char} *path: 文件路径
 * @return        {char *} :文件名指针
 **/
char *get_file_name(char *path)
{
    char *s = path;
    while (*s != '\0')
    {
        s++;
    }
    while (*s != '/' && (*s != '\\') && (s >= path))
    {
        s--;
    }
    return s + 1;
}
/**
 * @brief        : 字符串复制函数
 * @param         {char *} dest:目的地址
 * @param         {char *} src:源字符串地址
 * @return        {*}
 **/
void kernel_strcpy(char *dest, const char *src)
{
    if (!dest || !src)
    {
        return;
    }
    while (*dest && *src)
    {
        *dest = *src;
    }
    *dest = '\0';
}
/**
 * @brief        : 指定大小字符串复制
 * @param         {char *} dest:目的地址
 * @param         {char *} src:源字符串地址
 * @param         {int} size:大小
 * @return        {*}
 **/
void kernel_strncpy(char *dest, const char *src, int size)
{
    if (!dest || !src || !size)
    {
        return;
    }

    char *d = dest;
    const char *s = src;

    while ((size-- > 0) && (*s))
    {
        *d++ = *s++;
    }

    if (size == 0)
    {

        *(d - 1) = '\0';
    }
    else
    {
        *d = '\0';
    }
}

/**
 * @brief        : 字符串比较函数
 * @param         {char *} s1: 字符串1
 * @param         {char *} s2: 字符串2
 * @param         {int} size: 需要比较的字符串长度
 * @return        {*}相等返回0 ,否则返回-1
 **/
int kernel_strncmp(const char *s1, const char *s2, int size)
{
    if (!s1 || !s2)
    {
        return -1;
    }
    while (*s1 && *s2 && (*s1 == *s2) && size--)
    {
        s1++;
        s2++;
    }
    return !((*s1 == '\0') || (*s2 == '\0') || (*s1 == *s2));
}

/**
 * @brief        : 字符串的长度
 * @param         {char *} str: 字符串
 * @return        {*} 字符串的长度
 **/
int kernel_strlen(const char *str)
{
    if (!str)
    {
        return 0;
    }
    const char *c = str;
    int len = 0;
    while (*c++)
    {
        len++;
    }
    return len;
}

/**
 * @brief        : 内存复制函数
 * @param         {void *} dest: 目的地址
 * @param         {void *} src: 源地址
 * @param         {int} size: 需要复制的字节大小
 * @return        {*}
 **/
void kernel_memcpy(void *dest, void *src, int size)
{
    if (!dest || !src || !size)
    {
        return;
    }

    uint8_t *s = (uint8_t *)src;
    uint8_t *d = (uint8_t *)dest;
    while (size--)
    {
        *d++ = *s++;
    }
}
/**
 * @brief        : 内存设置函数
 * @param         {void *} dest: 目的地址
 * @param         {uint8_t} v: 设置成的数据
 * @param         {int} size: 字节大小
 * @return        {*}
 **/
void kernel_memset(void *dest, uint8_t v, int size)
{
    if (!dest || !size)
    {
        return;
    }
    uint8_t *d = dest;
    while (size--)
    {
        *d++ = v;
    }
}
/**
 * @brief        : 内存比较函数
 * @param         {void *} d1:数据d1的地址
 * @param         {void *} d2:数据d2的地址
 * @param         {int} size: 比较的字节大小
 * @return        {*}d1=d2返回0，相等返回非0;
 **/
int kernel_memcmp(void *d1, void *d2, int size)
{
    if (!d1 || !d2 || !size)
    {
        return 1;
    }
    uint8_t *p_d1 = d1;
    uint8_t *p_d2 = d2;
    while (size--)
    {
        if (*p_d1++ != *p_d2++)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief        : 将格式化输出到buf中
 * @param         {char} *buf: 输出字符缓冲区
 * @param         {char} *fmt: 格式化字符串
 * @return        {*}
 **/
void kernel_sprintf(char *buf, const char *fmt, ...)
{
    va_list args;                          // 可变参数存储变量
    kernel_memset(buf, '\0', sizeof(buf)); // 清空缓冲区
    va_start(args, fmt);                   // 将fmt后的可变参数存储到args中
    kernel_vsprintf(buf, fmt, args);       // 将可变参数放入缓冲区
    va_end(args);
}

void kernel_itoa(char *buf, int num, int base)
{
    // 转换字符索引[-15, -14, ...-1, 0, 1, ...., 14, 15]
    static const char *num2ch = {"FEDCBA9876543210123456789ABCDEF"};
    char *p = buf;
    int old_num = num;

    // 仅支持部分进制
    if ((base != 2) && (base != 8) && (base != 10) && (base != 16))
    {
        *p = '\0';
        return;
    }
    // 只支持十进制负数
    int signed_num = 0;
    if ((num < 0) && (base == 10))
    {
        *p++ = '-';
        signed_num = 1;
    }

    if (signed_num)
    {
        do
        {
            char ch = num2ch[num % base + 15];
            *p++ = ch;
            num /= base;
        } while (num);
    }
    else
    {
        uint32_t u_num = (uint32_t)num;
        do
        {
            char ch = num2ch[u_num % base + 15];
            *p++ = ch;
            u_num /= base;
        } while (u_num);
    }
    *p-- = '\0';

    // 将转换结果逆序，生成最终的结果
    char *start = (!signed_num) ? buf : buf + 1;
    while (start < p)
    {
        char ch = *start;
        *start = *p;
        *p-- = ch;
        start++;
    }
}

/**
 * @brief        : 根据格式化字符串将args参数填入buf中
 * @param         {char} *buf: 输出缓冲区
 * @param         {char} *fmt: 格式化字符串
 * @param         {va_list} args: 参数
 * @return        {*}
 **/
void kernel_vsprintf(char *buffer, const char *fmt, va_list args)
{
    enum
    {
        NORMAL,
        READ_FMT
    } state = NORMAL;
    char ch;
    char *curr = buffer;
    while ((ch = *fmt++))
    {
        switch (state)
        {
        // 普通字符
        case NORMAL:
            if (ch == '%')
            {
                state = READ_FMT;
            }
            else
            {
                *curr++ = ch;
            }
            break;
        // 格式化控制字符，只支持部分
        case READ_FMT:
            if (ch == 'd')
            {
                int num = va_arg(args, int);
                kernel_itoa(curr, num, 10);
                curr += kernel_strlen(curr);
            }
            else if (ch == 'x')
            {
                int num = va_arg(args, int);
                kernel_itoa(curr, num, 16);
                curr += kernel_strlen(curr);
            }
            else if (ch == 'c')
            {
                char c = va_arg(args, int);
                *curr++ = c;
            }
            else if (ch == 's')
            {
                const char *str = va_arg(args, char *);
                int len = kernel_strlen(str);
                while (len--)
                {
                    *curr++ = *str++;
                }
            }
            state = NORMAL;
            break;
        }
    }
}

/**
 * @brief        : 断言失败处理函数，用于停机
 * @param         {char} *file: 文件名
 * @param         {int} line: 代码行号
 * @param         {char} *func: 函数名
 * @param         {char} *cond: 表达式
 * @return        {*}
 **/
void pannic(const char *file, int line, const char *func, const char *cond)
{
    log_printf("ASSERT FAILED! %s\n", cond);
    log_printf("File is : %s \nLine is : %d\nFunc is : %s\n", file, line, func);
    for (;;)
    {
        hlt();
    }
}