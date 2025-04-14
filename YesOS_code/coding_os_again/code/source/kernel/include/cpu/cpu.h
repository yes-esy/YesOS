/**
 * @FilePath     : /code/source/kernel/include/cpu/cpu.h
 * @Description  :  cpu x86相关配置数据结构
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-14 21:22:23
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef CPU_H
#define CPU_H

#include "comm/types.h"

#pragma pack(1)
/**
 * GDT描述符
 */
typedef struct _segment_desc_t
{
    uint16_t limit15_0; // 界限
    uint16_t base15_0;  // 低15位基址
    uint8_t base23_16;  // 中8位基址
    uint16_t attr;      // 属性
    uint8_t base31_24;  // 高8位地址
} segment_desc_t;

/**
 * 中断门描述符
 */
typedef struct _gate_desc_t
{
    uint16_t offset15_0;  // 低16位偏移地址
    uint16_t selector;    // 选择子
    uint16_t attr;        // 属性
    uint16_t offset31_16; // 高16位偏移

} gate_desc_t;
#pragma pack()

#define SEG_G (1 << 15)        // 段界限的单位。1 - 界限的单位是4KB，0 - 字节。
#define SEG_D (1 << 14)        // 指示代码或栈的大小是32位还是16位，1 - 32位，0-16位
#define SEG_P_PRESENT (1 << 7) // 段是否存在，1存在，0不存在

#define SEG_DPL0 (0 << 5) // 特权级0，最高特权级
#define SEG_DPL3 (3 << 5) // 特权级3,最低特权级

#define SEG_S_SYSTEM (0 << 4) // 是否位系统段,如调用门或者中断
#define SEG_S_NORMAL (1 << 4) // 普通的代码段或数据段

#define SEG_TYPE_CODE (1 << 3) // 指定其为代码段
#define SEG_TYPE_DATA (0 << 3) // 指定其为数据段
#define SEG_TYE_RW (1 << 1)    // 是否可写可读,不设置为只读

#define GATE_TYPE_INT (0XE << 8) // 门类型
#define GATE_P_PRESENT (1 << 15) // 中断门是否存在
#define GATE_DPL0 (0 << 13)      // 特权级0
#define GATE_DPL3 (3 << 13)      // 特权级3

// cpu 初始化
void cpu_init(void);
// gdt表设置
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);
void gate_desc_set(gate_desc_t *desc, uint16_t selector, uint32_t offset, uint16_t attr);
#endif
