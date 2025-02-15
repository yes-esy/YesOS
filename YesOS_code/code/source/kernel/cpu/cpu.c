/**
 * cpu.c - CPU相关函数
 * @author yes
 * @date 2024-02-05
 */
#include "comm/cpu_instr.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "os_cfg.h"
#include "ipc/mutex.h"
#include "core/syscall.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE]; // GDT表

static mutex_t mutex;

/**
 * segment_desc_set - 设置段描述符
 * @param selector 选择子
 * @param base 基地址
 * @param limit 段限长
 * @param attr 属性
 */
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr)
{
    segment_desc_t *desc = gdt_table + selector / sizeof(segment_desc_t);

    if (limit > 0xfffff)
    {
        attr |= 0x8000;
        limit /= 0x1000;
    }

    desc->limit15_0 = limit & 0xffff;
    desc->base15_0 = base & 0xffff;
    desc->base23_16 = (base >> 16) & 0xff;
    desc->attr = attr | ((limit >> 16) & 0xf) << 8;
    desc->base31_24 = (base >> 24) & 0xff;
}

/**
 * gate_desc_set - 设置门描述符
 * @param desc 描述符
 * @param selector 选择子
 * @param offset 偏移
 * @param attr 属性
 */
void gate_desc_set(gate_desc_t *desc, uint16_t selector, uint32_t offset, uint16_t attr)
{
    desc->offset15_0 = offset & 0xffff;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xffff;
}

int gdt_alloc_desc(void)
{
    // irq_state_t state = irq_enter_protection();
    mutex_lock(&mutex);
    for (int i = 1; i < GDT_TABLE_SIZE; i++)
    {
        if (gdt_table[i].attr == 0)
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

void gdt_free_sel(int sel)
{
    mutex_lock(&mutex);
    gdt_table[sel / sizeof(segment_desc_t)].attr = 0;
    mutex_unlock(&mutex);
}

/**
 * init_gdt - 初始化GDT
 */
void init_gdt(void)
{
    for (int i = 0; i < GDT_TABLE_SIZE; i++)
    {
        segment_desc_set(i * sizeof(segment_desc_t), 0, 0, 0);
    }
    // 数据段
    segment_desc_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);

    // 只能用非一致代码段，以便通过调用门更改当前任务的CPL执行关键的资源访问操作
    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);

    // 调用门
    gate_desc_set((gate_desc_t *)(gdt_table + (SELECTOR_SYSCALL >> 3)),
                  KERNEL_SELECTOR_CS,
                  (uint32_t)exception_handler_syscall,
                  GATE_P_PRESENT | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_PARAM_COUNT);

    // 加载gdt
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}

/**
 * cpu_init - 初始化CPU
 */
void cpu_init(void)
{
    mutex_init(&mutex);
    init_gdt();
}

void switch_to_tss(int tss_sel)
{
    far_jump(tss_sel, 0);
}