/**
 * @FilePath     : /code/source/kernel/include/tools/klib.h
 * @Description  :  工具库头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 15:24:29
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef KLIB_H
#define KLIB_H

#include "comm/types.h"
#include <stdarg.h>

/**
 * @brief        : 向下取整到边界整数倍
 * @param         {uint32_t} size: 需要转换的大小
 * @param         {uint32_t} bound: 边界（必须为2的幂）
 * @return        {uint32_t} : 对齐后的结果
 **/
static inline uint32_t down2(uint32_t size, uint32_t bound)
{
    return size & ~(bound - 1);
}
/**
 * @brief        : 向上取整到边界整数倍
 * @param         {uint32_t} size: 需要转换的大小
 * @param         {uint32_t} bound: 边界（必须为2的幂）
 * @return        {uint32_t} : 对齐后的结果
 **/
static inline uint32_t up2(uint32_t size, uint32_t bound)
{
    return (size + bound - 1) & ~(bound - 1);
}

/**
 * @brief        : 字符串复制函数
 * @param         {char *} dest:目的地址
 * @param         {char *} src:源字符串地址
 * @return        {*}
 **/
void kernel_strcpy(char *dest, const char *src);
/**
 * @brief        : 指定大小字符串复制
 * @param         {char *} dest:目的地址
 * @param         {char *} src:源字符串地址
 * @param         {int} size:大小
 * @return        {*}
 **/
void kernel_strncpy(char *dest, const char *src, int size);

/**
 * @brief        : 字符串比较函数
 * @param         {char *} s1: 字符串1
 * @param         {char *} s2: 字符串2
 * @param         {int} size: 需要比较的字符串长度
 * @return        {*}s1 > s2 返回 1,相等返回0 ,否则返回-1
 **/
int kernel_strncmp(const char *s1, const char *s2, int size);

/**
 * @brief        : 字符串的长度
 * @param         {char *} str: 字符串
 * @return        {*} 字符串的长度
 **/
int kernel_strlen(const char *str);

/**
 * @brief        : 内存复制函数
 * @param         {void *} dest: 目的地址
 * @param         {void *} src: 源地址
 * @param         {int} size: 需要复制的字节大小
 * @return        {*}
 **/
void kernel_memcpy(void *dest, void *src, int size);

/**
 * @brief        : 内存设置函数
 * @param         {void *} dest: 目的地址
 * @param         {uint8_t} v: 设置成的数据
 * @param         {int} size: 字节大小
 * @return        {*}
 **/
void kernel_memset(void *dest, uint8_t v, int size);

/**
 * @brief        : 内存比较函数
 * @param         {void *} d1:数据d1的地址
 * @param         {void *} d2:数据d2的地址
 * @param         {int} size: 比较的字节大小
 * @return        {*}d1>d2返回1，相等返回0否则返回-1;
 **/
int kernel_memcmp(void *d1, void *d2, int size);

/**
 * @brief        : 将格式化输出到buf中
 * @param         {char} *buf: 输出字符缓冲区
 * @param         {char} *fmt: 格式化字符串
 * @return        {*}
 **/
void kernel_sprintf(char *buf, const char *fmt, ...);

/**
 * @brief        : 计算指针数组中的参数个数
 * @param         {char **} start: 起始地址
 * @return        {int} : 参数数量
 **/
int strings_count(char **start);
/**
 * @brief        : 从文件路径中获取文件名
 * @param         {char} *path: 文件路径
 * @return        {char *} :文件名指针
 **/
char *get_file_name(char *path);
/**
 * @brief        : 将args中的参数根据格式化字符串fmt填入缓冲区buf中
 * @param         {char} *buf: 字符缓冲区
 * @param         {char} *fmt: 格式化字符串
 * @param         {va_list} args: 参数列表
 * @return        {*}
 **/
void kernel_vsprintf(char *buf, const char *fmt, va_list args);
#ifndef RELASE
#define ASSERT(expr) \
    if (!(expr))     \
        pannic(__FILE__, __LINE__, __func__, #expr);

/**
 * @brief        : 断言失败处理函数，用于停机
 * @param         {char} *file: 文件名
 * @param         {int} line: 代码行号
 * @param         {char} *func: 函数名
 * @param         {char} *cond: 表达式
 * @return        {*}
 **/
void pannic(const char *file, int line, const char *func, const char *cond);

#else
#define ASSERT(expr) ((void)0)
#endif
#endif