/**
 * @FilePath     : /code/source/kernel/core/task.c
 * @Description  : 进程相关实现函数包括(调度,)
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-07 20:29:39
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "core/memory.h"
static task_manager_t task_manager; // 全局任务管理器

/**
 * @brief        : 返回下一个将要运行的进程,从就绪对列中取，若为空则运行空闲进程
 * @return        {task_t*} 下一运行任务的指针
 **/
task_t *task_next_run(void)
{
    if (list_count(&task_manager.ready_list) == 0)
    {
        return &task_manager.idle_task;
    }
    list_node_t *task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node); // 取出对应的队列头部进程
}

/**
 * @brief        : 返回当前进程的指针
 * @return        {task_t*}
 **/
task_t *task_current(void)
{
    return task_manager.curr_task;
}

/**
 * @brief        : 从队列头部取进程执行
 * @return        {*}
 * @todo         : 后续改进
 **/
void task_dispatch(void)
{

    irq_state_t state = irq_enter_protection();
    task_t *to = task_next_run();
    if (to != task_manager.curr_task) // 接下来需要运行的进程是否为当前进程
    {
        task_t *from = task_current();
        task_manager.curr_task = to;
        to->state = TASK_RUNNING;
        task_switch_from_to(from, to);
    }
    irq_leave_protection(state);
}
/**
 * @brief        : 将进程设为就绪状态,将当前进程插入到就绪队列,修改进程状态为就绪态
 * @param         {task_t} *task:
 * @return        {*}
 **/
void task_set_ready(task_t *task)
{
    if (task == &task_manager.idle_task) // 非空闲进程
    {
        return;
    }
    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}
/**
 * @brief        : 当前进程放弃cpu的使用权，将当前进程加入到就绪队列尾部，调度下一进程
 * @return        {*}
 **/
int sys_sched_yield(void)
{
    irq_state_t state = irq_enter_protection();
    if (list_count(&task_manager.ready_list) > 1) // 是否有进程
    {
        task_t *curr_task = task_current(); // 当前运行进程

        task_set_block(curr_task); // 阻塞当前进程
        task_set_ready(curr_task); // 当前进程加入就绪队列
        task_dispatch();           // 切换到队列头部的进程运行
    }

    irq_leave_protection(state);
    return 0;
}
/**
 * @brief        : 阻塞当前进程，将当前进程从就绪队列中移除
 * @param         {task_t} *task:
 * @return        {*}
 **/
void task_set_block(task_t *task)
{
    if (task == &task_manager.idle_task) // 空闲进程
    {
        return;
    }
    list_remove(&task_manager.ready_list, &task->run_node);
}

/**
 * @brief        : 对指定任务的TSS进行初始化包括设置GDT表项,寄存器设置,任务入口地址
 * @param         {task_t *} task: 需要运行的任务
 * @param         {uint32_t} entry: 入口地址
 * @param         {uint32_t} esp: 栈顶指针
 * @return        {int} 成功为0 ,失败为-1
 **/
static int tss_init(task_t *task, uint32_t entry, uint32_t esp)
{
    int tss_sel = gdt_alloc_desc(); // 分配一个空闲表项
    if (tss_sel < 0)
    {
        log_printf("alloc tss failed!!!\n");
        return -1;
    }
    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t),
                     SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS);

    kernel_memset(&task->tss, 0, sizeof(tss_t)); // 清零 , 第一次运行无关紧要
    task->tss.eip = entry;                       // 当前任务没有运行过,所以eip为当前任务的入口地址
    task->tss.esp = task->tss.esp0 = esp;        // esp0特权级0 , 设置栈地址

    // 平坦模型只有两个段cs和ds 其中ss , es , ds , fs , gs 设置为ds
    task->tss.ss = task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = KERNEL_SELECTOR_DS;

    // 设置cs
    task->tss.cs = KERNEL_SELECTOR_CS;
    task->tss.iomap = 0;
    // eflags
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF;
    uint32_t page_dir = memory_create_uvm();
    if(page_dir == 0)
    {
        gdt_free_sel(tss_sel);
        return -1;
    }
    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;
    return 0;
}

/**
 * @brief        : 进程初始化
 * @param         {task_t *}       task: 需要运行的进程的指针
 * @param         {const char *}   name : 进程名称
 * @param         {uint32_t}       entry: 入口地址(解析elf的得到的入口地址)
 * @param         {uint32_t}       esp: 栈顶指针
 * @return        {*}
 **/
int task_init(task_t *task, const char *name, uint32_t entry, uint32_t esp)

{
    ASSERT(task != (task_t *)0);
    tss_init(task, entry, esp);

    kernel_memcpy((void *)task->name, (void *)name, TASK_NAME_SIZE);

    task->state = TASK_CREATED;

    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->slice_ticks = task->time_ticks;
    task->sleep_ticks = 0; // 没有延时

    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    irq_state_t state = irq_enter_protection();

    task_set_ready(task); // 将进程设置为就绪状态

    list_insert_last(&task_manager.task_list, &task->all_node);

    irq_leave_protection(state);

    // uint32_t *p_esp = (uint32_t *)esp;
    // if (p_esp)
    // {
    //     *(--p_esp) = entry;
    //     *(--p_esp) = 0;
    //     *(--p_esp) = 0;
    //     *(--p_esp) = 0;
    //     *(--p_esp) = 0;
    //     task->stack = p_esp;
    // }
    return 0;
}

void simple_switch(uint32_t **from, uint32_t *to);
/**
 * @brief        : 实现从一个任务到另一个任务的切换
 * @param         {task_t *} from: 当前任务
 * @param         {task_t *} to: 切换到的任务
 * @return        {*}
 **/
void task_switch_from_to(task_t *from, task_t *to)
{
    switch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}

static uint32_t idle_task_stack[IDLE_TASK_SIZE]; // 空闲进程栈空间

/**
 * @brief        : 空闲进程所运行的任务
 * @return        {*}
 **/
static void idle_task_entry()
{
    log_printf("idle task running");
    for (;;)
    {
        hlt();
    }
}
/**
 * @brief        : 初始化任务(进程)管理器,包括初始化就绪,延时,进程队列,初始化空闲进程,将当前进程设为null
 * @return        {*}
 **/
void task_manager_init(void)
{
    list_init(&task_manager.ready_list); // 就绪队列
    list_init(&task_manager.task_list);  // 进程队列
    list_init(&task_manager.sleep_list); // 延时队列
    task_manager.curr_task = (task_t *)0;
    task_init(&task_manager.idle_task,
              "idle task",
              (uint32_t)idle_task_entry,
              (uint32_t)(idle_task_stack + IDLE_TASK_SIZE));
    task_manager.curr_task = (task_t *)0;
}

/**
 * @brief        : 初始化OS中的第一个任务
 * @return        {*}
 **/
void task_first_init(void)
{
    task_init(&task_manager.first_task, "first task", 0, 0);
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;
}

/**
 * @brief        : 返回OS中的第一个任务
 * @return        {*}
 **/
task_t *task_first_task(void)
{
    return &task_manager.first_task;
}
/**
 * @brief        :
 * @return        {*}
 **/
void task_time_ticks(void)
{
    task_t *curr_task = task_current();

    if (--curr_task->slice_ticks == 0) // 当前进程运行时间片已到达
    {
        curr_task->slice_ticks = curr_task->time_ticks;
        task_set_block(curr_task); //
        task_set_ready(curr_task);
    }

    // 扫描延时队列
    list_node_t *curr = list_first(&task_manager.sleep_list);
    while (curr)
    {
        task_t *task = list_node_parent(curr, task_t, run_node);
        list_node_t *next = list_node_next(curr);
        if (--task->sleep_ticks == 0) // 如果当前进程的延时已经到达
        {
            task_set_wakeup(task); // 唤醒
            task_set_ready(task);  // 状态设置为就绪
        }
        curr = next;
    }
    task_dispatch();
}
/**
 * @brief        : 将进程设置为延时(睡眠),设置延时时间,当前进程的状态为延时,并将其插入到延时队列
 * @param         {task_t} *task: 需要延时的进程(任务)
 * @param         {uint32_t} ticks: 延时(睡眠)的时间
 * @return        {*}
 **/
void task_set_sleep(task_t *task, uint32_t ticks)
{
    if (ticks == 0)
    {
        return;
    }
    task->sleep_ticks = ticks; // 设置延时时间
    task->state = TASK_SLEEP;  // 设置进程状态

    list_insert_last(&task_manager.sleep_list, &task->run_node); // 插入延时队列
}
/**
 * @brief        : 唤醒进程,从延时队列中移除该进程
 * @param         {task_t} *task: 需要唤醒的进程(任务)
 * @return        {*}
 **/
void task_set_wakeup(task_t *task) // 唤醒进程
{
    list_remove(&task_manager.sleep_list, &task->run_node); // 从延时队列中移除该结点
}

/**
 * @brief        : 将当前进程延时
 * @param         {uint32_t} ms: 需要延时的时间
 * @return        {*}
 **/
void sys_sleep(uint32_t ms) // 进程延时
{
    irq_state_t state = irq_enter_protection();

    task_set_block(task_manager.curr_task); // 阻塞当前进程（从就绪队列中移除当前进程）

    task_set_sleep(task_manager.curr_task, (ms + (OS_TICKS_MS - 1)) / OS_TICKS_MS); // 将当前进程设置为延时

    // 进程切换
    task_dispatch();

    irq_leave_protection(state);
}
