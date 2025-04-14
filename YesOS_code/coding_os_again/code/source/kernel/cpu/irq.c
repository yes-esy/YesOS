/**
 * @FilePath     : /code/source/kernel/cpu/irq.c
 * @Description  :  中断处理相关头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-14 21:36:05
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"

#define IDT_TABLE_NR 128 // 表项数量为128

void exception_handler_unknown(void);       // 中断处理函数
static gate_desc_t idt_table[IDT_TABLE_NR]; // IDT表

static void do_default_handler(const char *msg)
{
    for (;;)
    {
    }
}

void do_handler_unknown(void)
{
    do_default_handler("unknown exception!!!");
}

/**
 * @brief        : 中断初始化,初始化中断向量表
 * @return        {*}
 **/
void irq_init(void)
{
    for (int i = 0; i < IDT_TABLE_NR; i++)
    {
        gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (uint32_t)exception_handler_unknown,
                      GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    }
    lidt((uint32_t)idt_table, sizeof(idt_table));
}