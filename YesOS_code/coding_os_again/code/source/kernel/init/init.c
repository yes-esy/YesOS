/**
 * @FilePath     : /code/source/kernel/init/init.c
 * @Description  :  内核初始化文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-30 17:13:33
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "init.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "core/task.h"
#include "tools/list.h"
#include "ipc/sem.h"
#include "core/memory.h"
#include "dev/console.h"
#include "dev/keyboard.h"
static boot_info_t *init_boot_info; // 启动信息

static sem_t sem;

/**
 * @brief        : 内核初始化
 * @param         {boot_info_t} *boot_info: 启动信息
 * @return        {*}
 **/
void kernel_init(boot_info_t *boot_info)
{
    init_boot_info = boot_info;
    cpu_init();
    log_init();             // 打印初始化
    console_init();         // 控制台初始化
    memory_init(boot_info); // 内存初始化

    irq_init(); // 中断初始化
    time_init();
    task_manager_init();
    kbd_init(); // 键盘初始化
}

static task_t init_task; // 初始任务
// static task_t first_task; // 第一个任务
// static uint32_t first_task_stack[1024];
// void first_task_entry(void)
// {
//     int count = 0;
//     for (;;)
//     {
//         sem_wait(&sem);
//         log_printf("first_task_entry , count is %d", count++);
//         // sys_sched_yield();
//         // sys_sleep(10000);
//     }
// }

void list_test()
{
    list_t list;
    list_init(&list);

    log_printf("list: first = 0x%x , last = 0x%x ,count = %d\n",
               list_first(&list), list_last(&list), list_count(&list));

    list_node_t nodes[5];

    for (int i = 0; i < 5; i++)
    {
        list_node_t *node = nodes + i;
        log_printf("insert head to list : %d , 0x%x \n", i, (uint32_t)node);
        list_insert_first(&list, node);
    }

    log_printf("list: first = 0x%x , last = 0x%x ,count = %d\n",
               list_first(&list), list_last(&list), list_count(&list));

    list_init(&list);
    for (int i = 0; i < 5; i++)
    {
        list_node_t *node = nodes + i;
        log_printf("insert last to list : %d , 0x%x \n", i, (uint32_t)node);
        list_insert_last(&list, node);
    }

    log_printf("list: first = 0x%x , last = 0x%x ,count = %d\n",
               list_first(&list), list_last(&list), list_count(&list));

    // remove first
    for (int i = 0; i < 5; i++)
    {
        list_node_t *node = list_remove_first(&list);
        log_printf("remove head to list : %d , 0x%x \n", i, (uint32_t)node);
    }

    log_printf("list: first = 0x%x , last = 0x%x ,count = %d\n",
               list_first(&list), list_last(&list), list_count(&list));

    // remove node
    list_init(&list);
    for (int i = 0; i < 5; i++)
    {
        list_node_t *node = nodes + i;
        list_insert_last(&list, node);
    }

    for (int i = 0; i < 5; i++)
    {
        list_node_t *node = nodes + i;
        list_remove(&list, node);
        log_printf("remove node to list : %d , 0x%x \n", i, (uint32_t)node);
    }
    log_printf("list: first = 0x%x , last = 0x%x ,count = %d\n",
               list_first(&list), list_last(&list), list_count(&list));
}
/**
 * @brief        : 跳转到第一个进程
 * @return        {void}
 **/
void move_to_first_task(void)
{
    task_t *curr = task_current(); // 取当前进程
    ASSERT(curr != 0);             // 不为空
    tss_t *tss = &(curr->tss);
    __asm__ __volatile__(
        "push %[ss]\n\t"
        "push %[esp]\n\t"
        "push %[eflags]\n\t"
        "push %[cs]\n\t"
        "push %[eip]\n\t"
        "iret" ::[ss] "r"(tss->ss),
        [esp] "r"(tss->esp),
        [eflags] "r"(tss->eflags),
        [cs] "r"(tss->cs),
        [eip] "r"(tss->eip));
}

void init_main()
{
    // int a = 3 / 0;
    log_printf("Kernel is running . . .");
    log_printf("Version:%s", OS_VERSION);
    // task_init(&first_task, "init task", (uint32_t)first_task_entry, (uint32_t)&first_task_stack[1024]); // x86栈地址由高到低增长 ,同时init_task需要一个单独的栈空间。
    task_first_init();
    // list_test();
    // sem_init(&sem, 0);   // 初始化信号量
    // irq_enable_global(); // 测试打开全局中断
    // int count = 0;
    // for (;;)
    // {
    //     log_printf("init main , count is %d", count++);
    //     sem_signal(&sem);
    // sys_sched_yield();
    // sys_sleep(1000);
    // }

    move_to_first_task();
}