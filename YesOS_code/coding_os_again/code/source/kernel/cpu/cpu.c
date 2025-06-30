/**
 * @FilePath     : /code/source/kernel/cpu/cpu.c
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-19 21:34:51
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "cpu/cpu.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
// gdt_table gdt表
static segment_desc_t gdt_table[GDT_TABLE_SIZE];

static mutex_t mutex;
/**
 * @brief        : 对gdt表中指定的表项设置
 * @param         {int} selector: 选择子，gdt表中的某一个表项
 * @param         {uint32_t} base: 基地址
 * @param         {uint32_t} limit: 界限 20位
 * @param         {uint16_t} attr: 属性
 * @return        {void}
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
 * @brief        : 分配一个空闲gdt表项
 * @return        {int} 选择子
 **/
int gdt_alloc_desc(void)
{
    // irq_state_t state = irq_enter_protection();
    mutex_lock(&mutex);
    // 跳过第0项
    for (int i = 1; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_t *desc = gdt_table + i;
        if (desc->attr == 0) // 该表项为空闲
        {
            // irq_leave_protection(state);
            mutex_unlock(&mutex);
            return i * sizeof(segment_desc_t);
        }
    }
    // irq_leave_protection(state);
    mutex_unlock(&mutex);
    return -1;
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
 * @brief        : 切换至TSS,即跳转实现任务切换
 * @param         {uint16_t} tss_selector: 对应的tss选择子
 * @return        {*}
 **/
void switch_to_tss(uint32_t tss_selector)
{
    far_jump(tss_selector, 0);
}

/**
 * @brief        : gdt表项释放
 * @param         {int} sel: 选择子
 * @return        {*}
 **/
void gdt_free_sel(int sel)
{
    mutex_lock(&mutex);
    gdt_table[sel/sizeof(segment_desc_t)].attr = 0;
    mutex_unlock(&mutex);
}
/**
 * @brief        : cpu初始化
 * @return        {*}
 **/
void cpu_init(void)
{
    init_gdt();         // gdt表初始化
    mutex_init(&mutex); // 互斥锁初始化
}