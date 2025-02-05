/**
 * 功能：32位代码，完成多任务的运行
 * author:yes
 */
#include "os.h"

// 类型定义
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;


void do_syscall(int func, char * str,char color)
{
    static int row = 1;
    if(func ==2)
    {
        unsigned short * dest = (unsigned short *) 0xb8000 + 80 * row; 
        while (*str)
        {
            *dest++ = *str++ | (color<<8);
        }
        row = (row >= 25) ? 0 : row + 1;
        // 加点延时，让显示慢下来
        for (int i = 0; i < 0xFFFFFF; i++)
            ;
    }
    
}
/**
 * @brief 系统调用，在屏幕上显示字符串
 */
void sys_show(char *str, char color)
{
    const unsigned long sys_gate_addr[] = {0, SYSCALL_SEL}; // 使用特权级0

    // 采用调用门, 这里只支持5个参数
    // 用调用门的好处是会自动将参数复制到内核栈中，这样内核代码很好取参数
    // 而如果采用寄存器传递，取参比较困难，需要先压栈再取
    __asm__ __volatile__("push %[color];push %[str];push %[id];lcalll *(%[gate])\n\n" ::[color] "m"(color), [str] "m"(str), [id] "r"(2), [gate] "r"(sys_gate_addr));
}

/**
 * @brief 任务0 特权级3
 */
void task_0(void)
{
    char * message = "task 0:1234";
    uint8_t color = 0;
    for(;;)
    {
        sys_show(message,color++);
    }
}
/*
void delay(uint32_t ticks)
{
    volatile uint32_t i;
    for (i = 0; i < ticks; i++);
}

void task_0(void)
{
    uint8_t color = 0x1E;                  // 初始颜色（黄色+蓝色背景）
    unsigned short *dest = (unsigned short *)0xb8000;
    char *message = "Hello OS!";
    int len = 9;
    int pos = 80 * 12 + 35;                // 屏幕中央位置 (80列 x 25行)

    // 清屏：填充空格，采用白底黑字
    for (int i = 0; i < 80 * 25; i++) {
        dest[i] = ' ' | (0x07 << 8);
    }

    for (;;) {
        // 在屏幕中央打印消息，使用当前设置的颜色
        for (int i = 0; i < len; i++) {
            dest[pos + i] = message[i] | (color << 8);
        }
        delay(500000);

        // 更新颜色，循环变化
        color = (color + 1) & 0xFF;

        // 清除原来的文字区域以便下次重新绘制
        for (int i = 0; i < len; i++) {
            dest[pos + i] = ' ' | (0x07 << 8);
        }
    }
}
*/

/**
 * @brief 任务1
 */
void task_1(void)
{
    char *message = "task 1:5678";
    uint8_t color = 0;
    for (;;)
    {
        sys_show(message, color--);
    }
}

#define MAP_ADDR (0x80000000) // 要映射的虚拟地址

/**
 * @brief 系统页表
 * 将0x0-4MB虚拟地址映射到0x0-4MB物理地址，做恒等映射
 */
#define PDE_P (1 << 0)  // 页目录项存在
#define PDE_W (1 << 1)  // 页目录项可写
#define PDE_U (1 << 2)  // 页目录项用户态
#define PDE_PS (1 << 7) // 页目录项4M页

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))); // 页表所在的物理页

// 页目录项和页表项
static uint32_t pg_table[1024] __attribute__((aligned(4096))) = {PDE_U}; // 页表
uint32_t pg_dir[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_PS | PDE_W | PDE_U, // PDE_PS，开启4MB的页，恒等映射
};

/**
 * @brief 任务0和任务1的栈空间
 */
uint32_t task0_dpl3_stack[1024], task0_dpl0_stack[1024],task1_dpl0_stack[1024], task1_dpl3_stack[1024];

/**
 * @brief 任务0的LDT表
 */
struct
{
    uint16_t limit_l, base_l, basehl_attr, base_limit;
} task0_ldt_table[2] __attribute__((aligned(8))) = {
    // 0x00cffa000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，32位代码段，界限4G
    [TASK_CODE_SEG / 8] = {0xffff, 0x0000, 0xfa00, 0x00cf},
    // 0x00cff3000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，数据段，界限4G，可读写
    [TASK_DATA_SEG / 8] = {0xffff, 0x0000, 0xf300, 0x00cf},
};

/**
 * @brief 任务1的LDT
 */
struct
{
    uint16_t limit_l, base_l, basehl_attr, base_limit;
} task1_ldt_table[2] __attribute__((aligned(8))) = {
    // 0x00cffa000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，32位代码段，界限4G
    [TASK_CODE_SEG / 8] = {0xffff, 0x0000, 0xfa00, 0x00cf},
    // 0x00cff3000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，数据段，界限4G，可读写
    [TASK_DATA_SEG / 8] = {0xffff, 0x0000, 0xf300, 0x00cf},
};

/**
 * @brief 任务0的任务状态段
 */
uint32_t task0_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,
    (uint32_t)task0_dpl0_stack + 4 * 1024,
    KERNEL_DATA_SEG,
    /* 后边不用使用 */ 0x0,
    0x0,
    0x0,
    0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pg_dir,
    (uint32_t)task_0 /*入口地址*/,
    0x202,
    0xa,
    0xc,
    0xd,
    0xb,
    (uint32_t)task0_dpl3_stack + 4 * 1024 /* 栈 */,
    0x1,
    0x2,
    0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    // APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, (uint32_t), 0x0,
    TASK_DATA_SEG,
    TASK_CODE_SEG,
    TASK_DATA_SEG,
    TASK_DATA_SEG,
    TASK_DATA_SEG,
    TASK_DATA_SEG,
    TASK0_LDT_SEG,
    0x0,
};
/**
 * @brief 任务1的TSS
 */

uint32_t task1_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,
    (uint32_t)task1_dpl0_stack + 4 * 1024,
    KERNEL_DATA_SEG,
    /* 后边不用使用 */ 0x0,
    0x0,
    0x0,
    0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pg_dir,
    (uint32_t)task_1 /*入口地址*/,
    0x202,
    0xa,
    0xc,
    0xd,
    0xb,
    (uint32_t)task1_dpl3_stack + 4 * 1024 /* 栈 */,
    0x1,
    0x2,
    0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    // APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,
    TASK_DATA_SEG,
    TASK_CODE_SEG,
    TASK_DATA_SEG,
    TASK_DATA_SEG,
    TASK_DATA_SEG,
    TASK_DATA_SEG,
    TASK1_LDT_SEG,
    0x0,
};
// IDT表
struct
{
    uint16_t offset_l, selector, attr, offset_h;
} idt_table[256] __attribute__((aligned(8))) = {1};

// GDT表
struct
{
    uint16_t limit_l, base_l, basehl_attr, base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    // 0x00cf9a000000ffff -- 从0地址开始，P存在，DPL=0,Type=非系统段，32位代码段（非一致代码段），界限4G
    [KERNEL_CODE_SEG / 8] = {0xffff, 0X0000, 0x9a00, 0x00cf},
    // 0x00cf92000000ffff -- 从0地址开始，P存在，DPL=0,Type=非系统段，32位数据段，界限4G,可读写
    [KERNEL_DATA_SEG / 8] = {0xffff, 0X0000, 0x9200, 0x00cf},

    // // 创建的任务的代码段与数据段
    [APP_CODE_SEG / 8] = {0xffff, 0X0000, 0xfa00, 0x00cf},
    [APP_DATA_SEG / 8] = {0xffff, 0X0000, 0xf300, 0x00cf},

    // 任务0的TSS
    [TASK0_TSS_SEG / 8] = {0x0068, 0X0000, 0xe900, 0x0},
    // 任务1的TSS
    [TASK1_TSS_SEG / 8] = {0x0068, 0X0000, 0xe900, 0x0},
    // 系统调用
    [SYSCALL_SEG / 8] = {0x0000, KERNEL_CODE_SEG, 0xec03, 0},

    // 任务0的LDT
    [TASK0_LDT_SEG / 8] = {sizeof(task0_ldt_table) - 1, 0X0, 0xe200, 0x00cf},
    // 任务1的LDT
    [TASK1_LDT_SEG / 8] = {sizeof(task1_ldt_table) - 1, 0X0, 0xe200, 0x00cf},

};

void outb(uint8_t data, uint16_t port)
{
    __asm__ __volatile__("outb %[v], %[p]" ::[p] "d"(port), [v] "a"(data));
}

void task_sched(void)
{
    static int task_tss = TASK0_TSS_SEG;

    // 更换当前任务的tss，然后切换过去
    task_tss = (task_tss == TASK0_TSS_SEG) ? TASK1_TSS_SEG : TASK0_TSS_SEG;
    uint32_t addr[] = {0, task_tss};
    __asm__ __volatile__("ljmpl *(%[a])" ::[a]"r"(addr));
}

//申明函数
void syscall_handler(void);
void timer_init(void);

void os_init (void) {
    // 初始化8259中断控制器，打开定时器中断
    outb(0x11, 0x20);       // 开始初始化主芯片
    outb(0x11, 0xA0);       // 初始化从芯片
    outb(0x20, 0x21);       // 写ICW2，告诉主芯片中断向量从0x20开始
    outb(0x28, 0xa1);       // 写ICW2，告诉从芯片中断向量从0x28开始
    outb((1 << 2), 0x21);   // 写ICW3，告诉主芯片IRQ2上连接有从芯片
    outb(2, 0xa1);          // 写ICW3，告诉从芯片连接g到主芯片的IRQ2上
    outb(0x1, 0x21);        // 写ICW4，告诉主芯片8086、普通EOI、非缓冲模式
    outb(0x1, 0xa1);        // 写ICW4，告诉主芯片8086、普通EOI、非缓冲模式
    outb(0xfe, 0x21);       // 开定时中断，其它屏幕
    outb(0xff, 0xa1);       // 屏幕所有中断

    // 设置定时器，每100ms中断一次
    int tmo = (1193180 / 10);      // 时钟频率为1193180
    outb(0x36, 0x43);               // 二进制计数、模式3、通道0
    outb((uint8_t)tmo, 0x40);
    outb(tmo >> 8, 0x40);

    // 添加中断
    idt_table[0x20].offset_h = (uint32_t)timer_init >> 16;
    idt_table[0x20].offset_l = (uint32_t)timer_init & 0xffff;
    idt_table[0x20].selector = KERNEL_CODE_SEG;
    idt_table[0x20].attr = 0x8E00;      // 存在，DPL=0, 中断门

    // 添加任务和系统调用
    gdt_table[TASK0_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task0_tss;
    gdt_table[TASK1_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task1_tss;
    gdt_table[SYSCALL_SEG / 8].limit_l = (uint16_t)(uint32_t)syscall_handler;

    gdt_table[TASK0_LDT_SEG / 8].base_l = (uint16_t)(uint32_t)task0_ldt_table;
    gdt_table[TASK1_LDT_SEG / 8].base_l = (uint32_t)task1_ldt_table;

    // 虚拟内存
    // 0x80000000开始的4MB区域的映射
    pg_dir[MAP_ADDR >> 22] = (uint32_t)pg_table | PDE_P | PDE_W | PDE_U;
    pg_table[(MAP_ADDR >> 12) & 0x3FF] = (uint32_t)map_phy_buffer| PDE_P | PDE_W|PDE_U;
};