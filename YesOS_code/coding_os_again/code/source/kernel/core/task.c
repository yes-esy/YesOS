/**
 * @FilePath     : /code/source/kernel/core/task.c
 * @Description  : 进程相关实现函数包括(调度,)
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 13:15:16
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
#include "cpu/mmu.h"
#include "core/syscall.h"
#include "comm/elf.h"
#include "fs/fs.h"
#include "fs/file.h"

static task_manager_t task_manager; // 全局任务管理器
static task_t task_table[TASK_NR];  // 用户进程表
static mutex_t task_table_mutex;    // 进程表互斥访问锁
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
int sys_yield(void)
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
 * @brief        : 回收当前进程的资源
 * @param         {int *} satus: 传入的参数
 * @return        {int} :   返回进程的pid
 **/
int sys_wait(int *status)
{
    task_t *curr_task = task_current(); // 当前进程
    for (;;)
    {
        mutex_lock(&task_table_mutex);    // 上锁
        for (int i = 0; i < TASK_NR; i++) // 遍历进程表
        {
            task_t *task = task_table + i; // 取出该进程
            if (task->parent != curr_task) // 非当前进程子进程,跳过
            {
                continue;
            }

            if (task->state == TASK_ZOMBIE) // 僵尸进程
            {
                int pid = task->pid;                              // 取出进程id
                *status = task->status;                           // 取出状态值
                memory_destory_uvm(task->tss.cr3);                // 释放页表
                memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE); // 释放栈空间
                kernel_memset(task, 0, sizeof(task_t));           // 清空进程
                mutex_unlock(&task_table_mutex);                  // 解锁
                return pid;                                       // 返回子进程pid
            }
        }
        mutex_unlock(&task_table_mutex); // 没有找到,解锁
        // 没有找到处于僵尸状态的子进程,当前进程等待，切换进程
        irq_state_t irq_state = irq_enter_protection(); // 保护
        task_set_block(curr_task);                      // 阻塞当前进程
        curr_task->state = TASK_WAIT;                   // 设置为等待状态
        task_dispatch();                                // 切换另一个进程运行
        irq_leave_protection(irq_state);                // 退出保护
    }

    return 0;
}
/**
 * @brief        : 退出当前进程
 * @param         {int} status: 状态
 * @return        {void}
 **/
void sys_exit(int status)
{
    task_t *curr_task = task_current(); // 获取当前进程

    for (int fd = 0; fd < TASK_OPEN_FILE_NR; fd++) // 扫描进程文件表并清空
    {
        file_t *file = curr_task->file_table[fd]; // 取出文件指针
        if (file != (file_t *)0)                  // 不为空
        {
            sys_close(fd);                           // 关闭当前文件
            curr_task->file_table[fd] = (file_t *)0; // 清空
        }
    }
    // 处理孤儿进程：将当前进程的所有子进程转交给 first_task 管理
    int child_is_died = 0;         // 标记是否有子进程处于僵尸状态
    mutex_lock(&task_table_mutex); // 访问进程表
    for (int i = 0; i < TASK_NR; i++)
    {
        task_t *task = task_table + i; // 取出当前进程
        if (task->parent == curr_task) // 当前进程的子进程
        {
            task->parent = &task_manager.first_task; // 将当前进程的子进程的父进程设置为first_task
            if (task->state)                         // 当前进程子进程也处于僵死状态
            {
                child_is_died = 1; // 标记有僵尸子进程需要 first_task 回收
            }
        }
    }
    mutex_unlock(&task_table_mutex); // 解锁进程表

    irq_state_t state = irq_enter_protection(); // 关中断保护
    // 如果有僵尸子进程转交给了 first_task，且当前进程的父进程不是 first_task
    task_t *parent_task = curr_task->parent;                        // 取父进程
    if (child_is_died && (parent_task != &task_manager.first_task)) // 子进程处于僵死状态且父进程不为first_task
    {
        if (task_manager.first_task.state == TASK_WAIT) // first_task 正在等待子进程
        {
            task_set_ready(&task_manager.first_task); // 唤醒 first_task 去回收僵尸子进程
        }
    }
    if (parent_task->state == TASK_WAIT) // 如果父进程正在等待当前进程结束7
    {
        task_set_ready(curr_task->parent); // 唤醒父进程
    }

    curr_task->state = TASK_ZOMBIE; // 当前进程濒死
    curr_task->status = status;     // 保存传递的status
    task_set_block(curr_task);      // 将当前进程阻塞
    task_dispatch();                // 切换进程
    irq_leave_protection(state);    // 关中断返回
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
 * @param         {int} flag: 1为系统进程,0为普通进程
 * @param         {uint32_t} entry: 入口地址
 * @param         {uint32_t} esp: 栈顶指针
 * @return        {int} 成功为0 ,失败为-1
 **/
static int tss_init(task_t *task, int flag, uint32_t entry, uint32_t esp)
{
    int tss_sel = gdt_alloc_desc(); // 分配一个空闲表项,
    if (tss_sel < 0)
    {
        log_printf("alloc tss failed!!!\n");
        return -1;
    }
    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t),
                     SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS); // 为该表项设置属性

    int code_sel, data_sel;
    if (flag & TASK_FLAGS_SYSTEM) // 特权级0
    {
        code_sel = KERNEL_SELECTOR_CS; // 设置代码段权限
        data_sel = KERNEL_SELECTOR_DS; // 设置数据段权限
    }
    else // 特权级3
    {
        code_sel = task_manager.app_code_sel | SEG_CPL3; // 设置代码段权限
        data_sel = task_manager.app_data_sel | SEG_CPL3; // 设置数据段权限
    }
    kernel_memset(&task->tss, 0, sizeof(tss_t)); // 清零 , 第一次运行无关紧要
    uint32_t kernel_stack = memory_alloc_page(); // 分配内核栈空间
    if (kernel_stack == 0)
    {
        goto tss_init_failed;
    }
    task->tss.eip = entry;                         // 当前任务没有运行过,所以eip为当前任务的入口地址
    task->tss.esp = esp;                           // esp特权级3 , 设置栈底地址
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE; // esp0特权级0 , 设置栈底地址

    // 平坦模型只有两个段cs和ds 其中ss , es , ds , fs , gs 设置为ds
    task->tss.ss = data_sel;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = data_sel;

    // 设置cs
    task->tss.cs = code_sel;
    task->tss.iomap = 0;
    // eflags
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF;
    uint32_t page_dir = memory_create_uvm(); // 页目录表
    if (page_dir == 0)
    {
        goto tss_init_failed;
    }
    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;
    return 0;
tss_init_failed:
    gdt_free_sel(tss_sel);
    return -1;
}

/**
 * @brief        : 进程初始化
 * @param         {task_t *} task: 需要运行的进程的指针
 * @param         {const char *} name : 进程名称
 * @param         {int} flag: 1为系统进程,0为普通进程
 * @param         {uint32_t} entry: 入口地址(解析elf的得到的入口地址)
 * @param         {uint32_t} esp: 栈顶指针
 * @return        {int} : 返回 0
 **/
int task_init(task_t *task, const char *name, int flag, uint32_t entry, uint32_t esp)
{
    ASSERT(task != (task_t *)0);
    tss_init(task, flag, entry, esp);

    kernel_memcpy((void *)task->name, (void *)name, TASK_NAME_SIZE);

    task->state = TASK_CREATED; // 创建态

    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->slice_ticks = task->time_ticks; // 最小时间片
    task->sleep_ticks = 0;                // 没有延时
    task->parent = (task_t *)0;
    task->heap_start = 0; // 堆地址为0,大小为0；
    task->heap_end = 0;
    task->pid = (uint32_t)task; // 设置为当前进程的地址
    task->status = 0;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    kernel_memset((void *)task->file_table, 0, sizeof(task->file_table)); // 清空打开文件表

    irq_state_t state = irq_enter_protection();

    // task_set_ready(task); // 将进程设置为就绪状态

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
/**
 * @brief        : 启动进程
 * @param         {task_t} *task: 需要启动的进程
 * @return        {*}
 **/
void task_start(task_t *task)
{
    irq_state_t state = irq_enter_protection();

    task_set_ready(task);

    irq_leave_protection(state);
}
/**
 * @brief        : 反初始化
 * @param         {task_t *} task: 进行反初始化的进程的指针
 * @return        {void}
 **/
void task_uninit(task_t *task)
{
    if (task->tss_sel) // 选择子有效
    {
        gdt_free_sel(task->tss_sel);
    }

    if (task->tss.esp0) // 以分配特权级为0的栈
    {
        memory_free_page(task->tss.esp - MEM_PAGE_SIZE); // 从起始地址开始
    }

    if (task->tss.cr3) // 释放页表
    {
        memory_destory_uvm(task->tss.cr3);
    }

    kernel_memset(task, 0, sizeof(task)); // 清空该任务
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
    // log_printf("idle task running\n");
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
    kernel_memset((void *)task_table, 0, sizeof(task_table));
    mutex_init(&task_table_mutex);

    int data_sel = gdt_alloc_desc(); // 分配一个数据段描述符
    // 初始化描述符
    segment_desc_set(data_sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYE_RW | SEG_D);
    task_manager.app_data_sel = data_sel; // 保存数据段描述符

    int code_sel = gdt_alloc_desc(); // 分配一个代码段描述符
    // 初始化描述符
    segment_desc_set(code_sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYE_RW | SEG_D);
    task_manager.app_code_sel = code_sel; // 保存代码段描述符

    list_init(&task_manager.ready_list); // 就绪队列
    list_init(&task_manager.task_list);  // 进程队列
    list_init(&task_manager.sleep_list); // 延时队列
    task_manager.curr_task = (task_t *)0;
    task_init(&task_manager.idle_task,
              "idle task",
              TASK_FLAGS_SYSTEM,
              (uint32_t)idle_task_entry,
              (uint32_t)(idle_task_stack + IDLE_TASK_SIZE));
    task_manager.curr_task = (task_t *)0;
    task_start(&task_manager.idle_task);
}

/**
 * @brief        : 初始化OS中的第一个任务
 * @return        {void}
 **/
void task_first_init(void)
{
    void first_task_entry(void);                                  // 第一个任务入口 线性地址
    extern uint8_t s_first_task[], e_first_task[];                // 物理起始地址与结束地址
    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task); // 搬运大小
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);
    uint32_t first_start = (uint32_t)first_task_entry;
    task_init(&task_manager.first_task, "first task",
              0, first_start,
              first_start + alloc_size); // 指定起始地址,first_start + alloc_size为栈低(由高向低增长)
    // 设置堆
    task_manager.first_task.heap_start = (uint32_t)e_first_task;
    task_manager.first_task.heap_end = (uint32_t)e_first_task;
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;
    mmu_set_page_dir(task_manager.first_task.tss.cr3); // 切换页目录表

    // 切换页目录表后,分配页表空间
    memory_alloc_page_for(first_start, alloc_size, PTE_P | PTE_W | PTE_U);
    // 分配完后拷贝
    kernel_memcpy((void *)first_start, (void *)s_first_task, copy_size);

    task_start(&task_manager.first_task);
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

static task_t *alloc_task(void)
{
    task_t *task = (task_t *)0;
    mutex_lock(&task_table_mutex);
    for (int i = 0; i < TASK_NR; i++) // 寻找空闲表项
    {
        task_t *curr = task_table + i;
        if (curr->name[0] == 0) // 空闲进程
        {
            task = curr;
            break;
        }
    }
    mutex_unlock(&task_table_mutex);
    return task;
}
/**
 * @brief        : 释放一个进程
 * @param         {task_t} *task: 需要释放的进程的指针
 * @return        {void}
 **/
static void free_task(task_t *task)
{
    mutex_lock(&task_table_mutex);
    task->name[0] = '\0';
    mutex_unlock(&task_table_mutex);
}
/**
 * @brief        : 将程序表头加载到页表中
 * @param         {int} file: 文件ID
 * @param         {Elf32_Phdr *} phdr: elf文件表头
 * @param         {uint32_t} page_dir: 加载目的页目录表
 * @return        {int} : 状态码:1成功,2失败
 **/
int load_phdr(int file, Elf32_Phdr *phdr, uint32_t page_dir)
{
    // 分配内存空间
    int err = memory_alloc_page_for_dir(page_dir, phdr->p_vaddr, phdr->p_memsz, PTE_P | PTE_U | PTE_W);
    if (err < 0) // 分配失败
    {
        log_printf("memory_alloc_page_for_dir err.\n");
        return -1;
    }
    if (sys_lseek(file, phdr->p_offset, 0) < 0) // 调整文件读写指针值偏移量的位置
    {
        log_printf("read file failed\n");
        return -1;
    }
    uint32_t vaddr = phdr->p_vaddr; // 起始地址
    uint32_t size = phdr->p_filesz; // 文件大小
    while (size > 0)                // 是否有数据量
    {
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size; // 拷贝数据量
        uint32_t paddr = memory_get_paddr(page_dir, vaddr);            // vaddr在pade_dir中的物理地址

        if (sys_read(file, (char *)paddr, curr_size) < curr_size) // 读取到实际的物理地址中
        {
            log_printf("read file failed.\n");
            return -1;
        }
        size -= curr_size;  //
        vaddr += curr_size; //
    }
    return 0;
}
/**
 * @brief        : 加载elf可执行文件
 * @param         {task_t *} task: 将elf文件加载到哪一个进程运行
 * @param         {char *} path: elf文件的路径
 * @param         {uint32_t} page_dir: elf运行的新的页目录表
 * @return        {uint32_t} : 入口地址
 **/
static uint32_t load_elf_file(task_t *task, const char *path, uint32_t page_dir)
{
    Elf32_Ehdr elf_hdr;           // elf文件头
    Elf32_Phdr elf_phdr;          // elf表项
    int file = sys_open(path, 0); // 只读方式打开文件
    if (file < 0)                 // 打开失败
    {
        log_printf("open failed.path = %s\n", path);
        goto load_failed; // 错误处理
    }
    int cnt = sys_read(file, (char *)&elf_hdr, sizeof(elf_hdr)); // 读取文件头
    if (cnt < sizeof(Elf32_Ehdr))                                // 读取失败
    {
        log_printf("elf hdr too small.\n");
        goto load_failed;
    }
    // 是否为elf文件
    if ((elf_hdr.e_ident[0] != ELF_MAGIC || elf_hdr.e_ident[1] != 'E') ||
        (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F'))
    {
        log_printf("check elf ident failed.\n");
        goto load_failed;
    }
    // 必须是可执行文件和针对386处理器的类型，且有入口
    if ((elf_hdr.e_type != ET_EXEC) || (elf_hdr.e_machine != ET_386) || (elf_hdr.e_entry == 0))
    {
        log_printf("check elf type or entry failed.\n");
        goto load_failed;
    }
    // 必须有程序头部
    if ((elf_hdr.e_phentsize == 0) || (elf_hdr.e_phoff == 0))
    {
        log_printf("none programe header\n");
        goto load_failed;
    }

    uint32_t e_phoff = elf_hdr.e_phoff;

    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize) // 遍历表项
    {
        if (sys_lseek(file, e_phoff, 0) < 0) // 调整读写指针
        {
            log_printf("read file failed\n");
            goto load_failed;
        }
        cnt = sys_read(file, (char *)&elf_phdr, sizeof(elf_phdr)); // 读到elf_phdr中
        if (cnt < sizeof(elf_phdr))
        {
            log_printf("read file failed\n");
            goto load_failed;
        }

        // 主要判断是否是可加载的类型，并且要求加载的地址必须是用户空间
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE))
        {
            continue;
        }
        int err = load_phdr(file, &elf_phdr, page_dir); // 加载当前程序头到新页表中
        if (err < 0)                                    // 加载失败
        {
            log_printf("load program hdr failed\n");
            goto load_failed;
        }
        // task->heap_start最终指向bss区末端地址
        task->heap_start = elf_phdr.p_vaddr + elf_phdr.p_memsz;
        task->heap_end = task->heap_start; // 堆大小为0
    }
    sys_close(file);
    return elf_hdr.e_entry;

load_failed:
    if (file >= 0)
    {
        sys_close(file);
    }

    return 0;
}
/**
 * @brief        : 将参数拷贝到目的地址
 * @param         {char *} to: 目的地址
 * @param         {uint32_t} page_dir: 对应页目录表
 * @param         {int} argc: 拷贝参数的个数
 * @param         {char **} argv: 字符串数组
 * @return        {int} : 状态码 -1失败,0成功
 **/
static int copy_args(char *to, uint32_t page_dir, int argc, char **argv)
{
    // 初始化参数
    task_args_t task_args;
    task_args.argc = argc;
    task_args.argv = (char **)(to + sizeof(task_args_t)); // 指向指针数组的起始位置

    char *dest_arg = to + sizeof(task_args_t) + sizeof(char *) * argc;                              // 计算字符串存储区的起始位置
    char **dest_arg_tb = (char **)memory_get_paddr(page_dir, (uint32_t)(to + sizeof(task_args_t))); // 获取指针数组在目标页表中的物理地址
    for (int i = 0; i < argc; i++)
    {
        char *from = argv[i];
        int len = kernel_strlen(from) + 1;                                                 // 字符串长度
        int err = memory_copy_uvm_data((uint32_t)dest_arg, page_dir, (uint32_t)from, len); // 拷贝字符串内容到目标位置
        ASSERT(err >= 0);
        dest_arg_tb[i] = dest_arg; // 在指针数组中记录这个字符串的虚拟地址
        dest_arg += len;           // 移动到下一个位置
    }
    return memory_copy_uvm_data((uint32_t)to, page_dir, (uint32_t)&task_args, sizeof(task_args_t)); // 将数据拷贝到page_dir中对应to的物理地址处
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
/**
 * @brief        : 获取进程ID
 * @return        {int} : 进程ID
 **/
int sys_getpid(void)
{
    return task_current()->pid;
}
/**
 * @brief        : 子进程复制当前进程打开的文件
 * @param         {task_t *} child_task: 子进程
 * @return        {void}
 **/
static void copy_opened_files(task_t *child_task)
{
    task_t *parent_task = task_current();
    for (int i = 0; i < TASK_OPEN_FILE_NR; i++) // 扫描父进程的进程文件表
    {
        file_t *file = parent_task->file_table[i]; // 取出该文件
        if (file != (file_t *)0)                   // 不为空
        {
            file_incr_ref(file);               // 子进程打开该文件增加文件使用次数
            child_task->file_table[i] = file; // 复制到子进程
        }
    }
}

/**
 * @brief        : 为当前进程创建一个子进程
 * @return        {int} : 子进程PID号
 **/
int sys_fork(void)
{
    task_t *parent_task = task_current(); // 当前进程为父进程
    task_t *child_task = alloc_task();    // 分配子进程
    if (child_task == (task_t *)0)
    {
        goto fork_failed;
    }
    syscall_frame_t *frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));                          // 父进程的栈信息(寄存器)
    int err = task_init(child_task, parent_task->name, 0, frame->eip, frame->esp + sizeof(uint32_t) * SYSCALL_PARAM_COUNT); // 初始化子进程
    copy_opened_files(child_task);                                                                                          // 子进程复制父进程文件表
    tss_t *tss = &child_task->tss;                                                                                          // 子进程的tss
    tss->eax = 0;                                                                                                           // 子进程返回值
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;

    tss->esi = frame->esi;
    tss->ebp = frame->ebp;
    tss->edi = frame->edi;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child_task->parent = parent_task; // 设置子进程的父进程

    if ((tss->cr3 = memory_copy_uvm(parent_task->tss.cr3)) < 0) // 单独分配页表
    {
        goto fork_failed;
    }
    task_start(child_task);
    return child_task->pid; // 返回子进程pid给父进程

fork_failed:
    if (child_task)
    {
        task_uninit(child_task);
        free_task(child_task);
    }
    return -1;
}
/**
 * @brief        : 创建一个新的进程运行
 * @param         {char} *name: 应用程序路径
 * @param         {char} *: 参数
 * @param         {char} *: 环境变量
 * @return        {int} : 创建进程的PID
 **/
int sys_execve(char *path, char **argv, char **env)
{
    task_t *task = task_current(); // 当前运行的进程
    kernel_strncpy(task->name, get_file_name(path), TASK_NAME_SIZE);
    uint32_t old_page_dir = task->tss.cr3;       // 原页表
    uint32_t new_page_dir = memory_create_uvm(); // 创建新的页目录表
    if (!new_page_dir)                           // 创建失败
    {
        goto exce_failed;
    }
    uint32_t entry = load_elf_file(task, path, new_page_dir); // 加载elf文件,获取入口地址
    if (entry == 0)                                           // 加载失败
    {
        goto exce_failed;
    }
    uint32_t stack_top = MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE; // 栈顶
    int err = memory_alloc_page_for_dir(
        new_page_dir,                             // 新页表中分配
        MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE, // 起始地址
        MEM_TASK_STACK_SIZE,                      // 大小
        PTE_P | PTE_U | PTE_W);                   // 分配栈空间
    if (err < 0)                                  // 出现错误
    {
        goto exce_failed;
    }
    int argc = strings_count(argv);                               // 计算参数个数
    err = copy_args((char *)stack_top, new_page_dir, argc, argv); // 拷贝参数
    if (err < 0)
    {
        goto exce_failed;
    }
    syscall_frame_t *frame = (syscall_frame_t *)(task->tss.esp0 - sizeof(syscall_frame_t)); // 当前进程的栈信息(寄存器)
    frame->eip = entry;                                                                     // 更改返回的地址,返回到函数入口
    // 清空相关寄存器
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFLAGS_IF | EFLAGS_DEFAULT;
    frame->esp = stack_top - sizeof(uint32_t) * SYSCALL_PARAM_COUNT; // 设置栈帧顶部
    task->tss.cr3 = new_page_dir;                                    // 更新页表
    mmu_set_page_dir(new_page_dir);                                  // 更新cr3为新页表
    memory_destory_uvm(old_page_dir);                                // 释放原页表
    return 0;
exce_failed:
    if (new_page_dir)
    {
        task->tss.cr3 = old_page_dir;     // 恢复为原页表
        mmu_set_page_dir(old_page_dir);   // 恢复cr3为旧页表
        memory_destory_uvm(new_page_dir); // 释放新创建的页表
    }
    return -1;
}
/**
 * @brief        : 分配一个文件描述符
 * @param         {file_t} *file: 对应文件指针
 * @return        {int} : 成功则返回进程文件表分配给文件描述符的表项对应的索引,否则返回-1
 **/
int task_alloc_fd(file_t *file)
{
    task_t *task = task_current();              // 获取当前进程
    for (int i = 0; i < TASK_OPEN_FILE_NR; i++) // 扫描当前进程文件表
    {
        file_t *cur_file = task->file_table[i]; // 对应文件描述符
        if (cur_file == (file_t *)0)            // 空闲表项
        {
            task->file_table[i] = file; // 将该表项分配给该文件描述符
            return i;
        }
    }
    return -1; // 失败返回-1
}
/**
 * @brief        : 释放进程文件表中的文件
 * @param         {int} fd: 对应文件的索引
 * @return        {void}
 **/
void task_remove_fd(int fd)
{
    if ((fd >= 0) && (fd < TASK_OPEN_FILE_NR)) // 索引合法
    {
        task_current()->file_table[fd] = (file_t *)0; // 清空该表项
    }
}
/**
 * @brief        : 返回表项对应的指针
 * @param         {int} fd: 文件描述索引
 * @return        {file_t *} : 文件描述符指针(成功),失败返回0.
 **/
file_t *task_file(int fd)
{
    file_t *file = (file_t *)0;                // 记录当前
    if ((fd >= 0) && (fd < TASK_OPEN_FILE_NR)) // 索引合法
    {
        file = task_current()->file_table[fd]; // 取当前文件指针
    }
    return file;
}