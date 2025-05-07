/**
 * @FilePath     : /code/source/kernel/tools/log.c
 * @Description  :  日志输出实现
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-27 21:29:41
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "tools/log.h"
#include "comm/cpu_instr.h"
#include <stdarg.h>
#include "tools/klib.h"
#include "cpu/irq.h"

#define COM1_PORT 0x3F8

/**
 * @brief        : 日志输出初始化函数，对相应寄存器进行设置
 * @return        {*}
 **/
void log_init(void)
{
    outb(COM1_PORT + 1, 0x00); // 中断相关
    outb(COM1_PORT + 3, 0x80); // 发送速度
    outb(COM1_PORT + 0, 0x3);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0F);
}
/**
 * @brief        : 实现打印的功能(类似printf)
 * @param         {char} *fmt: 格式化字符串
 * @return        {*}
 **/
void log_printf(const char *fmt, ...)
{
    char str_buf[128];
    va_list args;                                  // 可变参数存储变量
    kernel_memset(str_buf, '\0', sizeof(str_buf)); // 清空缓冲区
    va_start(args, fmt);                           // 将fmt后的可变参数存储到args中
    kernel_vsprintf(str_buf, fmt, args);           // 将可变参数放入缓冲区
    va_end(args);
    irq_state_t state = irq_enter_protection(); // 进入临界区
    const char *p = str_buf;
    while (*p != '\0')
    {
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0) // 正在忙则等待
            ;
        outb(COM1_PORT, *p++); // 发送数据
    }
    outb(COM1_PORT, '\r');
    outb(COM1_PORT, '\n');
    irq_leave_protection(state); // 退出临界区
}
