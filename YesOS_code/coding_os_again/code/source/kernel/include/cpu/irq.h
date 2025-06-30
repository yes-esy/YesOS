/**
 * @FilePath     : /code/source/kernel/include/cpu/irq.h
 * @Description  :  中断处理相关头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-29 16:35:52
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef IRQ_H
#define IRQ_H
#include "comm/types.h"

// 中断序号
#define IRQ0_DE 0   // 除0异常
#define IRQ1_DB 1   // 调试异常
#define IRQ2_NMI 2  //
#define IRQ3_BP 3   //
#define IRQ4_OF 4   //
#define IRQ5_BR 5   //
#define IRQ6_UD 6   //
#define IRQ7_NM 7   //
#define IRQ8_DF 8   //
#define IRQ10_TS 10 //
#define IRQ11_NP 11 //
#define IRQ12_SS 12 //
#define IRQ13_GP 13 //
#define IRQ14_PF 14 //
#define IRQ16_MF 16 //
#define IRQ17_AC 17 //
#define IRQ18_MC 18 //
#define IRQ19_XM 19 //
#define IRQ20_VE 20 //

#define IRQ0_TIMER 0x20

// PIC控制器相关的寄存器及位2配置
#define PIC0_ICW1 0x20
#define PIC0_ICW2 0x21
#define PIC0_ICW3 0x21
#define PIC0_ICW4 0x21
#define PIC0_OCW2 0x20
#define PIC0_IMR 0x21

#define PIC1_ICW1 0xA0
#define PIC1_ICW2 0xA1
#define PIC1_ICW3 0xA1
#define PIC1_ICW4 0xA1
#define PIC1_OCW2 0xA0
#define PIC1_IMR 0xA1

#define PIC_ICW1_ICW4 (1 << 0)     // 需要初始化ICW4
#define PIC_ICW1_ALWAYS_1 (1 << 4) // 总为1的模式
#define PIC_ICW4_8086 (1 << 0)     // 8086模式

#define PIC_OCW2_EOI (1 << 5)

#define IRQ_PIC_START 0x20 // PIC起始中断号

#define ERR_PAGE_P (1 << 0)
#define ERR_PAGE_WR (1 << 1)
#define ERR_PAGE_US (1 << 1)

#define ERR_EXT (1 << 0)
#define ERR_IDT (1 << 1)

void irq_init(void); // 中断初始化函数

// 中断发生时异常信息保存在exception_frame_t中(无特权级)
typedef struct _exception_frame_t
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t num, error_code;
    uint32_t eip, cs, eflags;
    uint32_t esp3, ss3;
} exception_frame_t;

typedef void (*irq_handler_t)(exception_frame_t *frame); // 中断处理类型irq_handler_t
int irq_install(int irq_num, irq_handler_t handler);     // 中断绑定

// 中断捕获
void exception_handler_divider(exception_frame_t *frame); // 除0异常
void exception_handler_divider(exception_frame_t *frame);
void exception_handler_Debug(exception_frame_t *frame);
void exception_handler_NMI(exception_frame_t *frame);
void exception_handler_breakpoint(exception_frame_t *frame);
void exception_handler_overflow(exception_frame_t *frame);
void exception_handler_bound_range(exception_frame_t *frame);
void exception_handler_invalid_opcode(exception_frame_t *frame);
void exception_handler_device_unavailable(exception_frame_t *frame);
void exception_handler_double_fault(exception_frame_t *frame);
void exception_handler_invalid_tss(exception_frame_t *frame);
void exception_handler_segment_not_present(exception_frame_t *frame);
void exception_handler_stack_segment_fault(exception_frame_t *frame);
void exception_handler_general_protection(exception_frame_t *frame);
void exception_handler_page_fault(exception_frame_t *frame);
void exception_handler_fpu_error(exception_frame_t *frame);
void exception_handler_alignment_check(exception_frame_t *frame);
void exception_handler_machine_check(exception_frame_t *frame);
void exception_handler_smd_exception(exception_frame_t *frame);
void exception_handler_virtual_exception(exception_frame_t *frame);
void exception_handler_control_exception(exception_frame_t *frame);

void irq_enable(int irq_num);  // 开启特定中断
void irq_disable(int irq_num); // 关闭特定中断

// 关闭于开启全局中断的声明
void irq_disable_global(void); // 关闭全局中断
void irq_enable_global(void);  // 开启全局中断

void pic_send_eoi(int irq);

typedef uint32_t irq_state_t;

irq_state_t irq_enter_protection(void);       // 进入临界区
void irq_leave_protection(irq_state_t state); // 退出临界区
#endif
