/**
 * @FilePath     : /code/source/kernel/cpu/cpu.c
 * @Description  : 
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-14 21:53:27
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "cpu/cpu.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
// gdt_table gdt表
static segment_desc_t gdt_table[GDT_TABLE_SIZE];

/**
 * @brief        : 对gdt表中指定的表项设置
 * @param         {int} selector: 选择子，gdt表中的某一个表项
 * @param         {uint32_t} base: 基地址
 * @param         {uint32_t} limit: 界限 20位
 * @param         {uint16_t} attr: 属性
 * @return        {*}
 **/
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr)
{
    segment_desc_t *desc = gdt_table + (selector >> 3);

    // 如果界限比较长，将长度单位换成4KB
    if (limit > 0xFFFFF)
    {
        attr |= 0x8000;
        limit /= 0x1000;
    }
    desc->limit15_0 = limit & 0xFFFF; 
    desc->base15_0 = base & 0xFFFF;
    desc->base23_16 = (base >> 16) & 0xFF;
    desc->attr = attr | (((limit >> 16) & 0xF) << 8);
    desc->base31_24 = (base >> 24) & 0xFF;
}

/**
 * @brief        : 对中断门类型的表项进行设置
 * @param         {gate_desc_t} *desc: 指定的表项
 * @param         {uint16_t} selector: 对应的处理程序所在的代码段的选择子
 * @param         {uint32_t} offset: 地址偏移量
 * @param         {uint16_t} attr: 属性
 * @return        {*}
**/
void gate_desc_set(gate_desc_t *desc, uint16_t selector, uint32_t offset, uint16_t attr)
{
    desc->offset15_0 = offset & 0xFFFF;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xFFFF;
}

/**
 * @brief        : gdt表初始化
 * @return        {*}
 **/
void init_gdt(void)
{
    for (int i = 0; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_set(i << 3, 0, 0, 0);
    }

    // 数据段
    segment_desc_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYE_RW | SEG_D | SEG_G);
    // 代码段
    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYE_RW | SEG_D | SEG_G);
    // 加载gdt
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}

/**
 * @brief        : cpu初始化
 * @return        {*}
 **/
void cpu_init(void)
{
    init_gdt(); // gdt表初始化
}