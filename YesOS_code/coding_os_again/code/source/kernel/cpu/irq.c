/**
 * @FilePath     : /code/source/kernel/cpu/irq.c
 * @Description  :  中断处理相关头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-07 19:38:24
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"
#include "tools/log.h"

#define IDT_TABLE_NR 128 // 表项数量为128

void exception_handler_unknown(void);       // 中断处理函数
static gate_desc_t idt_table[IDT_TABLE_NR]; // IDT表

/**
 * @brief        :  打印寄存器异常信息
 * @param         {exception_frame_t} *frame: 寄存器信息
 * @return        {*}
 **/
static void dump_core_regs(exception_frame_t *frame)
{
    // 打印CPU寄存器相关内容
    log_printf("IRQ: %d, error code: %d.", frame->num, frame->error_code);
    log_printf("CS: %d\nDS: %d\nES: %d\nSS: %d\nFS: %d\nGS:%d",
               frame->cs, frame->ds, frame->es, frame->ds, frame->fs, frame->gs);
    log_printf("EAX: 0x%x\n"
               "EBX: 0x%x\n"
               "ECX: 0x%x\n"
               "EDX: 0x%x\n"
               "EDI: 0x%x\n"
               "ESI: 0x%x\n"
               "EBP: 0x%x\n"
               "ESP: 0x%x\n",
               frame->eax, frame->ebx, frame->ecx, frame->edx,
               frame->edi, frame->esi, frame->ebp, frame->esp);
    log_printf("EIP: 0x%x\nEFLAGS: 0x%x\n", frame->eip, frame->eflags);
}
/**
 * @brief        : 默认异常处理函数
 * @param         {exception_frame_t} *frame:保存一些寄存器的值(异常信息)
 * @param         {char} *msg: 异常提示
 * @return        {*}
 **/
static void do_default_handler(exception_frame_t *frame, const char *msg)
{
    log_printf("----------------------------------------");
    log_printf("IRQ/EXCEPTION HAPPEND: %s", msg);
    dump_core_regs(frame);
    for (;;)
    {
        hlt();
    }
}
/**
 * @brief        : 未知的异常处理函数
 * @param         {exception_frame_t} *frame: 保存在寄存器中的异常信息结构体
 * @return        {*}
 **/
void do_handler_unknown(exception_frame_t *frame)
{
    do_default_handler(frame, "unknown exception!!!");
}
/**
 * @brief        : 除0异常处理函数
 * @param         {exception_frame_t} *frame: 保存在寄存器中的异常信息结构体
 * @return        {*}
 **/
void do_handler_divider(exception_frame_t *frame)
{
    do_default_handler(frame, "Divder exception!!!");
}
/**
 * @brief        : 调试异常处理函数
 * @param         {exception_frame_t} *frame: 保存在寄存器中的异常信息结构体
 * @return        {*}
 **/
void do_handler_Debug(exception_frame_t *frame)
{
    do_default_handler(frame, "Debug exception!!!");
}
/**
 * @brief        : NMI异常处理函数
 * @param         {exception_frame_t} *frame: 保存在寄存器中的异常信息结构体
 * @return        {*}
 **/
void do_handler_NMI(exception_frame_t *frame)
{
    do_default_handler(frame, "NMI exception!!!");
}
/**
 * @brief        : 断点异常(?)处理函数
 * @param         {exception_frame_t} *frame: 保存在寄存器中的异常信息结构体
 * @return        {*}
 **/
void do_handler_breakpoint(exception_frame_t *frame)
{
    do_default_handler(frame, "breakpoint exception!!!");
}
void do_handler_overflow(exception_frame_t *frame)
{
    do_default_handler(frame, "overflow exception!!!");
}
void do_handler_bound_range(exception_frame_t *frame)
{
    do_default_handler(frame, "bound range exception!!!");
}
void do_handler_invalid_opcode(exception_frame_t *frame)
{
    do_default_handler(frame, "invalid opcode exception!!!");
}
void do_handler_device_unavailable(exception_frame_t *frame)
{
    do_default_handler(frame, "device unavailable exception!!!");
}
void do_handler_double_fault(exception_frame_t *frame)
{
    do_default_handler(frame, "double fault exception!!!");
}
void do_handler_invalid_tss(exception_frame_t *frame)
{
    do_default_handler(frame, "invalid tss exception!!!");
}
void do_handler_segment_not_present(exception_frame_t *frame)
{
    do_default_handler(frame, "segment not present exception!!!");
}
void do_handler_stack_segment_fault(exception_frame_t *frame)
{
    do_default_handler(frame, "segment fault exception!!!");
}
void do_handler_general_protection(exception_frame_t *frame)
{
    do_default_handler(frame, "general protection exception!!!");
}
void do_handler_page_fault(exception_frame_t *frame)
{
    do_default_handler(frame, "page fault exception!!!");
}
void do_handler_fpu_error(exception_frame_t *frame)
{
    do_default_handler(frame, "fpu error exception!!!");
}
void do_handler_alignment_check(exception_frame_t *frame)
{
    do_default_handler(frame, "alignment check exception!!!");
}
void do_handler_machine_check(exception_frame_t *frame)
{
    do_default_handler(frame, "machine check exception!!!");
}
void do_handler_smd_exception(exception_frame_t *frame)
{
    do_default_handler(frame, "smd exception!!!");
}
void do_handler_virtual_exception(exception_frame_t *frame)
{
    do_default_handler(frame, "virtual exception!!!");
}
void do_handler_control_exception(exception_frame_t *frame)
{
    do_default_handler(frame, "control exception!!!");
}
/**
 * @brief        : 初始化8259芯片,实现定时器中断。
 * @return        {*}
 **/
static void init_pic()
{
    // 第一块8259配置 边缘触发,级联需要配置icw4，8086模式
    outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    outb(PIC0_ICW2, IRQ_PIC_START); // 起始序号从0x20开始
    outb(PIC0_ICW3, 1 << 2);        // 主片PIC0_ICW3 有从片
    outb(PIC0_ICW4, PIC_ICW4_8086); // 普通全嵌套、非缓冲、自动结束8086模式

    // 第二块8259配置
    outb(PIC1_ICW1, PIC_ICW1_ICW4 | PIC_ICW1_ALWAYS_1);
    outb(PIC1_ICW2, IRQ_PIC_START + 8); // 从0x28开始
    outb(PIC1_ICW3, 2);                 // 没有从片，连接到主片的IRQ2上
    outb(PIC1_ICW4, PIC_ICW4_8086);     // 普通全嵌套、非缓冲、非自动结束、8086模式

    // // 禁止所有中断, 允许从PIC1传来的中断
    outb(PIC0_IMR, 0xFF & ~(1 << 2));
    outb(PIC1_IMR, 0xFF);
}

/**
 * @brief        : 通过写8259的ocw端口，来告诉操作系统irq_num对应的中断已经响应完了
 * @param         {int} irq_num: 中断号
 * @return        {*}
 **/
void pic_send_eoi(int irq_num)
{
    irq_num -= IRQ_PIC_START;

    if (irq_num >= 8)
    {
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    }

    outb(PIC0_OCW2, PIC_OCW2_EOI);
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

    // 安装异常处理函数
    irq_install(IRQ0_DE, (irq_handler_t)exception_handler_divider); // 除0异常
    irq_install(IRQ1_DB, (irq_handler_t)exception_handler_Debug);
    irq_install(IRQ2_NMI, (irq_handler_t)exception_handler_NMI);
    irq_install(IRQ3_BP, (irq_handler_t)exception_handler_breakpoint);
    irq_install(IRQ4_OF, (irq_handler_t)exception_handler_overflow);
    irq_install(IRQ5_BR, (irq_handler_t)exception_handler_bound_range);
    irq_install(IRQ6_UD, (irq_handler_t)exception_handler_invalid_opcode);
    irq_install(IRQ7_NM, (irq_handler_t)exception_handler_device_unavailable);
    irq_install(IRQ8_DF, (irq_handler_t)exception_handler_double_fault);
    irq_install(IRQ10_TS, (irq_handler_t)exception_handler_invalid_tss);
    irq_install(IRQ11_NP, (irq_handler_t)exception_handler_segment_not_present);
    irq_install(IRQ12_SS, (irq_handler_t)exception_handler_stack_segment_fault);
    irq_install(IRQ13_GP, (irq_handler_t)exception_handler_general_protection);
    irq_install(IRQ14_PF, (irq_handler_t)exception_handler_page_fault);
    irq_install(IRQ16_MF, (irq_handler_t)exception_handler_fpu_error);
    irq_install(IRQ17_AC, (irq_handler_t)exception_handler_alignment_check);
    irq_install(IRQ18_MC, (irq_handler_t)exception_handler_machine_check);
    irq_install(IRQ19_XM, (irq_handler_t)exception_handler_smd_exception);
    irq_install(IRQ20_VE, (irq_handler_t)exception_handler_virtual_exception);

    // 加载IDT表
    lidt((uint32_t)idt_table, sizeof(idt_table));

    init_pic(); // 初始化8259芯片
}

/**
 * @brief        :
 * @param         {int} irq_num:
 * @param         {irq_handler_t} handler:
 * @return        {*}
 **/
int irq_install(int irq_num, irq_handler_t handler)
{
    if (irq_num >= IDT_TABLE_NR)
    {
        return -1;
    }
    gate_desc_set(idt_table + irq_num, KERNEL_SELECTOR_CS, (uint32_t)handler, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    return 0;
}

/**
 * @brief        : 开启特定中断
 * @param         {int} irq_num: 特定中断号
 * @return        {*}
 **/
void irq_enable(int irq_num)
{
    if (irq_num < IRQ_PIC_START)
    {
        return;
    }
    irq_num -= IRQ_PIC_START;
    if (irq_num < 8)
    {
        // 第一块8259中的中断
        uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_num);
        outb(PIC0_IMR, mask);
    }
    else
    {
        // 第二块8259的中断
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

/**
 * @brief        : 关闭特定中断
 * @param         {int} irq_num: 特定中断号
 * @return        {*}
 **/
void irq_disable(int irq_num)
{
    if (irq_num < IRQ_PIC_START)
    {
        return;
    }
    irq_num -= IRQ_PIC_START;
    if (irq_num < 8)
    {
        // 第一块8259中的中断
        uint8_t mask = inb(PIC0_IMR) | (1 << irq_num);
        outb(PIC0_IMR, mask);
    }
    else
    {
        // 第二块8259的中断
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) | (1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

/**
 * @brief        : 全局中断关闭eflags置0
 * @return        {*}
 **/
void irq_disable_global(void)
{
    cli();
}

/**
 * @brief        : 全局中断开启eflags置1
 * @return        {*}
 **/
void irq_enable_global(void)
{
    sti();
}
/**
 * @brief        : 进入临界区,读取eflags的值，关中断
 * @return        {irq_state_t}eflags的值
 **/
irq_state_t irq_enter_protection(void)
{

    irq_state_t state = read_eflags();

    irq_disable_global();

    return state;
}
/**
 * @brief        : 退出临界区,将eflags原来的值写回
 * @param         {irq_state_t} state: 原eflags的值
 * @return        {*}
 **/
void irq_leave_protection(irq_state_t state)
{
    write_eflags(state);
}
