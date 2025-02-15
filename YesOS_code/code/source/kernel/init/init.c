/**
 * init.c - 内核初始化
 * @auther yes
 * @date 2025-02-05
 */
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "core/task.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "tools/list.h"
#include "ipc/sem.h"
#include "core/memory.h"
#include "dev/console.h"
#include "dev/kbd.h"
#include "fs/fs.h"

static sem_t sem;
/**
 * kernel_init - 内核初始化
 * @param boot_info 引导信息
 */
void kernel_init(boot_info_t *boot_info)
{
    ASSERT(boot_info->ram_region_count != 0);
    // ASSERT(3 < 2);
    cpu_init(); // 初始化CPU
    irq_init(); // 初始化中断idt
    log_init(); // 初始化日志

    memory_init(boot_info); // 初始化内存
    fs_init(); // 初始化文件系统

    time_init(); // 初始化定时器

    task_manager_init(); // 初始化任务管理器

    log_printf("init successfully");
    log_printf("welcome to YesOS , version is 0.0.1");
    log_printf("launch time: %s %s", __DATE__, __TIME__);

    // for(;;);
}

/**
 * @brief 移至第一个进程运行
 */
void move_to_first_task(void)
{
    task_t *curr = task_current();
    ASSERT(curr != 0);

    tss_t *tss = &(curr->tss);

    // 也可以使用类似boot跳loader中的函数指针跳转
    // 这里用jmp是因为后续需要使用内联汇编添加其它代码
    __asm__ __volatile__(
        // 模拟中断返回，切换入第1个可运行应用进程
        // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
        "push %[ss]\n\t"     // SS
        "push %[esp]\n\t"    // ESP
        "push %[eflags]\n\t" // EFLAGS
        "push %[cs]\n\t"     // CS
        "push %[eip]\n\t"    // ip
        "iret\n\t" ::[ss] "r"(tss->ss),
        [esp] "r"(tss->esp), [eflags] "r"(tss->eflags),
        [cs] "r"(tss->cs), [eip] "r"(tss->eip));
}

void init_main()
{
    // list_test();
    log_printf("Kernel is running...");
    log_printf("Version: %s", OS_VERSION);
    // int a = 3 / 0;
    task_first_init();
    move_to_first_task();
}