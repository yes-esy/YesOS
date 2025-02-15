/**
 * task.c - task management
 */

#include "comm/cpu_instr.h"
#include "core/task.h"
#include "tools/klib.h"
#include "tools/log.h"
#include "os_cfg.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "core/memory.h"
#include "applib/lib_syscall.h"
#include "fs/fs.h"
#include "comm/elf.h"

static task_manager_t task_manager;
static uint32_t idle_task_stack[IDLE_STACK_SIZE]; // 空闲任务堆栈
static task_t task_table[TASK_NR];                // 任务表
static mutex_t task_table_mutex;                  // 任务表互斥锁

int task_alloc_fd(file_t *file) // 返回文件描述符索引
{
    task_t *task = task_current();
    for (int i = 0; i < TASK_OFILE_NR; i++)
    {
        file_t *p = task->file_table[i];
        if (p == (file_t *)0)
        {
            task->file_table[i] = file;
            return i;
        }
    }
    return -1;
}
void task_remove_fd(int fd) // 释放文件描述符
{
    if ((fd >= 0) && (fd < TASK_OFILE_NR))
    {
        task_t *task = task_current();
        task->file_table[fd] = (file_t *)0;
    }
}
file_t *task_file(int fd) // 返回文件描述符
{
    if ((fd >= 0) && (fd < TASK_OFILE_NR))
    {
        task_t *task = task_current();
        return task->file_table[fd];
    }
    return (file_t *)0;
}

/**
 * @brief 初始化TSS
 */
static int tss_init(task_t *task, int flag, uint32_t entry, uint32_t esp)
{
    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0)
    {
        log_printf("tss_init: alloc tss descriptor failed\n");
        return -1;
    }

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t),
                     SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS);

    kernel_memset(&task->tss, 0, sizeof(tss_t));

    uint32_t kernel_stack = memory_alloc_page();
    if (kernel_stack == 0)
    {
        goto tss_init_failed;
    }

    int code_sel, data_sel;

    if (flag & TASK_FLAGS_SYSTEM)
    {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    }
    else
    {
        // 注意加了RP3,不然将产生段保护错误
        code_sel = task_manager.app_code_sel | SEG_RPL3;
        data_sel = task_manager.app_data_sel | SEG_RPL3;
    }

    task->tss.eip = entry;
    task->tss.esp = esp;
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss = data_sel;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = data_sel;
    task->tss.cs = code_sel;
    task->tss.eflags = EFALGS_IF | EFALGS_DEFAULT;
    uint32_t page_dir = memory_create_uvm();
    if (page_dir == -1)
    {
        gdt_free_sel(tss_sel);
        goto tss_init_failed;
    }
    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;
    return 0;

tss_init_failed:
    gdt_free_sel(tss_sel);

    if (kernel_stack)
    {
        memory_free_page(kernel_stack);
    }
    return -1;
}
/**
 * @brief 初始化任务
 */
int task_init(task_t *task, const char *task_name, int flag, uint32_t entry, uint32_t esp)
{
    ASSERT(task != (task_t *)0);
    // 任务字段初始化
    kernel_strncpy(task->name, task_name, TASK_NAME_SIZE);

    tss_init(task, flag, entry, esp);
    task->state = TASK_CREATED;
    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->slice_ticks = task->time_ticks;
    task->sleep_ticks = 0;
    task->pid = (uint32_t)task;
    task->parent = (task_t *)0;
    task->heap_start = 0;
    task->heap_end = 0;
    task->exit_status = 0;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    kernel_memset((void *)task->file_table, 0, sizeof(task->file_table));

    // 插入就绪队列中和所有的任务队列中
    irq_state_t state = irq_enter_protection();
    list_insert_last(&task_manager.task_list, &task->all_node);
    irq_leave_protection(state);
    // uint32_t *pesp = (uint32_t *)esp;

    // if (pesp)
    // {
    //     *(--pesp) = entry;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     task->stack = pesp;
    // }
    return 0;
}

void task_uninit(task_t *task)
{
    if (task->tss_sel)
    {
        gdt_free_sel(task->tss_sel);
    }

    if (task->tss.esp0)
    {
        memory_free_page(task->tss.esp - MEM_PAGE_SIZE);
    }

    if (task->tss.cr3)
    {
        memory_destroy_uvm(task->tss.cr3);
    }
    kernel_memset(task, 0, sizeof(task));
}
void task_start(task_t *task)
{
    irq_state_t state = irq_enter_protection();
    task_set_ready(task);
    irq_leave_protection(state);
}
void simple_switch(uint32_t **from, uint32_t *to);
void task_switch_from_to(task_t *from, task_t *to)
{
    switch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}

/**
 * @brief 空闲任务
 */
static void idle_task_entry(void)
{
    for (;;)
    {
        hlt();
    }
}

/**
 * @brief 任务管理器初始化
 */
void task_manager_init(void)
{

    kernel_memset(&task_manager, 0, sizeof(task_manager_t));
    mutex_init(&task_table_mutex);
    // 数据段和代码段，使用DPL3，所有应用共用同一个
    // 为调试方便，暂时使用DPL0
    int sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |
                         SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);
    task_manager.app_data_sel = sel;

    sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |
                         SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);
    task_manager.app_code_sel = sel;

    // 各队列初始化
    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);

    // 空闲任务初始化
    task_init(&task_manager.idle_task,
              "idle task",
              TASK_FLAGS_SYSTEM,
              (uint32_t)idle_task_entry,
              (uint32_t)(idle_task_stack + IDLE_STACK_SIZE)); // 里面的值不必要写

    task_manager.current_task = (task_t *)0;
    task_start(&task_manager.idle_task);
}
/**
 * @brief 初始进程的初始化
 * 没有采用从磁盘加载的方式，因为需要用到文件系统，并且最好是和kernel绑在一定，这样好加载
 * 当然，也可以采用将init的源文件和kernel的一起编译。此里要调整好kernel.lds，在其中
 * 将init加载地址设置成和内核一起的，运行地址设置成用户进程运行的高处。
 * 不过，考虑到init可能用到newlib库，如果与kernel合并编译，在lds中很难控制将newlib的
 * 代码与init进程的放在一起，有可能与kernel放在了一起。
 * 综上，最好是分离。
 */
void task_first_init(void)
{
    void first_task_entry(void);

    // 以下获得的是bin文件在内存中的物理地址
    extern uint8_t s_first_task[], e_first_task[];

    // 分配的空间比实际存储的空间要大一些，多余的用于放置栈
    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);

    uint32_t first_start = (uint32_t)first_task_entry;

    // 第一个任务代码量小一些，好和栈放在1个页面呢
    // 这样就不要立即考虑还要给栈分配空间的问题
    task_init(&task_manager.first_task, "first task", 0, first_start, first_start + alloc_size); // 里面的值不必要写
    task_manager.first_task.heap_start = (uint32_t)e_first_task;
    task_manager.first_task.heap_end = (uint32_t)e_first_task;
    task_manager.current_task = &task_manager.first_task;

    // 更新页表地址为自己的
    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    // 分配一页内存供代码存放使用，然后将代码复制过去
    memory_alloc_page_for(first_start, alloc_size, PTE_P | PTE_W | PTE_U);
    kernel_memcpy((void *)first_start, (void *)s_first_task, copy_size);

    // 启动进程
    task_start(&task_manager.first_task);
    // 写TR寄存器，指示当前运行的第一个任务
    write_tr(task_manager.first_task.tss_sel);
}

/**
 * @brief 返回初始任务
 */
task_t *task_first_task(void)
{
    return &task_manager.first_task;
}

/**
 * @brief 将任务插入就绪队列
 */
void task_set_ready(task_t *task)
{
    if (task == &task_manager.idle_task)
    {
        return;
    }
    task->state = TASK_READY;
    list_insert_last(&task_manager.ready_list, &task->run_node);
}

/**
 * @brief 将任务从就绪队列移除
 */
void task_set_block(task_t *task)
{
    if (task != &task_manager.idle_task)
    {
        list_remove(&task_manager.ready_list, &task->run_node);
    }
}

/**
 * @brief 返回下一个要运行的任务
 */
static task_t *task_next_run(void)
{
    // 如果没有任务，则运行空闲任务
    if (list_count(&task_manager.ready_list) == 0)
    {
        return &task_manager.idle_task;
    }

    // 普通任务
    list_node_t *task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

/**
 * @brief 返回当前任务
 */
task_t *task_current(void)
{
    return task_manager.current_task;
}

/**
 * @brief 任务调度 - 切换到下一个任务
 */
int sys_yield(void)
{
    irq_state_t state = irq_enter_protection();
    if (list_count(&task_manager.ready_list) > 1)
    {
        task_t *curr_task = task_current();

        task_set_block(curr_task); // 将当前任务从就绪队列移除
        task_set_ready(curr_task); // 将当前任务插入就绪队列尾部

        task_dispatch(); // 调度下一个任务
    }
    irq_leave_protection(state);
    return 0;
}

/**
 * @brief 任务调度
 */
void task_dispatch(void)
{
    task_t *to = task_next_run();
    if (to != task_manager.current_task)
    {
        task_t *from = task_manager.current_task;
        task_manager.current_task = to;

        to->state = TASK_RUNNING;
        task_switch_from_to(from, to);
    }
}

/**
 * @brief 时间片到
 */
void task_time_tick(void)
{
    task_t *curr_task = task_current();

    irq_state_t state = irq_enter_protection();
    if (--curr_task->slice_ticks == 0)
    {

        curr_task->slice_ticks = curr_task->time_ticks;

        task_set_block(curr_task); // 将当前任务从就绪队列移除
        task_set_ready(curr_task); // 将当前任务插入就绪队列尾部

        task_dispatch(); // 调度下一个任务
    }

    list_node_t *curr = list_first(&task_manager.sleep_list);

    while (curr)
    {
        list_node_t *next = list_node_next(curr);
        task_t *task = list_node_parent(curr, task_t, run_node);
        if (--task->sleep_ticks == 0)
        {
            task_set_wakeup(task);
            task_set_ready(task);
        }

        curr = next;
    }
    task_dispatch();
    irq_leave_protection(state);
}
void task_set_sleep(task_t *task, uint32_t ticks)
{
    if (ticks == 0)
    {
        return;
    }

    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;
    list_insert_last(&task_manager.sleep_list, &task->run_node);
}
void task_set_wakeup(task_t *task)
{
    list_remove(&task_manager.sleep_list, &task->run_node);
}

void sys_msleep(uint32_t ms)
{
    irq_state_t state = irq_enter_protection();

    task_t *curr_task = task_current();

    task_set_block(curr_task);

    task_set_sleep(curr_task, (ms + (OS_TICK_MS - 1)) / OS_TICK_MS);
    task_dispatch();
    irq_leave_protection(state);
}

int sys_getpid(void)
{
    task_t *task = task_current();
    return task->pid;
}

static task_t *alloc_task(void)
{
    task_t *task = (task_t *)0;
    mutex_lock(&task_table_mutex);
    for (int i = 0; i < TASK_NR; i++)
    {
        task_t *curr = task_table + i;
        if (curr->name[0] == 0)
        {
            task = curr;
            break;
        }
    }
    mutex_unlock(&task_table_mutex);
    return task;
}

static void free_task(task_t *task)
{
    mutex_lock(&task_table_mutex);
    // kernel_memset(task, 0, sizeof(task_t));
    task->name[0] = 0;
    mutex_unlock(&task_table_mutex);
}
static void copy_opened_files(task_t *child_task)
{
    task_t *parent_task = task_current();
    for (int i = 0; i < TASK_OFILE_NR; i++)
    {
        file_t *file = parent_task->file_table[i];
        if (file)
        {
            file_inc_ref(file);
            child_task->file_table[i] = file;
        }
    }
}
int sys_fork(void)
{
    task_t *parent_task = task_current();

    task_t *child_task = alloc_task();

    if (child_task == (task_t *)0)
    {
        goto fork_failed;
    }
    syscall_frame_t *parent_frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));
    int err = task_init(child_task, parent_task->name, 0, parent_frame->eip, parent_frame->esp + sizeof(uint32_t) * SYSCALL_PARAM_COUNT);
    if (err < 0)
    {
        goto fork_failed;
    }
    copy_opened_files(child_task);


    tss_t *child_tss = &child_task->tss;
    child_tss->eax = 0;
    child_tss->ebx = parent_frame->ebx;
    child_tss->ecx = parent_frame->ecx;
    child_tss->edx = parent_frame->edx;
    child_tss->esi = parent_frame->esi;
    child_tss->edi = parent_frame->edi;
    child_tss->ebp = parent_frame->ebp;

    child_tss->cs = parent_frame->cs;
    child_tss->ds = parent_frame->ds;
    child_tss->es = parent_frame->es;
    child_tss->fs = parent_frame->fs;
    child_tss->gs = parent_frame->gs;
    child_tss->eflags = parent_frame->eflags;

    child_task->parent = parent_task;

    if ((child_task->tss.cr3 = memory_copy_uvm(parent_task->tss.cr3)) < 0)
    {
        goto fork_failed;
    }
    task_start(child_task);
    return child_task->pid;
fork_failed:
    if (child_task)
    {
        task_uninit(child_task);
        free_task(child_task);
    }
    return -1;
}

static int load_phdr(int file, Elf32_Phdr *phdr, uint32_t page_dir)
{
    int err = memory_alloc_for_page_dir(
        page_dir,
        phdr->p_vaddr,
        phdr->p_memsz,
        PTE_P | PTE_U | PTE_W);
    if (err < 0)
    {
        log_printf("alloc memory failed\n");
        return -1;
    }
    if (sys_lseek(file, phdr->p_offset, 0) < 0)
    {
        log_printf("lseek file failed\n");
        return -1;
    }
    uint32_t vaddr = phdr->p_vaddr;
    uint32_t size = phdr->p_filesz;
    while (size > 0)
    {
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;
        uint32_t paddr = memory_get_paddr(page_dir, vaddr);

        if (sys_read(file, (char *)paddr, curr_size) < curr_size)
        {
            log_printf("read file failed\n");
            return -1;
        }
        size -= curr_size;
        vaddr += curr_size;
    }
    return 0;
}

static uint32_t load_elf_file(task_t *task, char *name, uint32_t page_dir)
{
    Elf32_Ehdr elf_hdr;
    Elf32_Phdr elf_phdr;

    int file = sys_open(name, 0);
    if (file < 0)
    {
        log_printf("open file %s failed\n", name);
        goto load_failed;
    }
    int cnt = sys_read(file, (char *)&elf_hdr, sizeof(Elf32_Ehdr));

    if (cnt < sizeof(Elf32_Ehdr))
    {
        log_printf("read file %s failed\n", name);
        goto load_failed;
    }

    if (elf_hdr.e_ident[0] != 0x7f ||
        elf_hdr.e_ident[1] != 'E' ||
        elf_hdr.e_ident[2] != 'L' ||
        elf_hdr.e_ident[3] != 'F')
    {
        log_printf("file %s is not a elf file\n", name);
        goto load_failed;
    }

    uint32_t e_phoff = elf_hdr.e_phoff;

    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize)
    {
        if (sys_lseek(file, e_phoff, 0) < 0)
        {
            log_printf("lseek file %s failed\n", name);
            goto load_failed;
        }
        cnt = sys_read(file, (char *)&elf_phdr, sizeof(elf_phdr));

        if (cnt < sizeof(elf_phdr))
        {
            log_printf("read file %s failed\n", name);
            goto load_failed;
        }
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr > MEMORY_TASK_BASE))
        {
            continue;
        }
    }
    int err = load_phdr(file, &elf_phdr, page_dir);
    if (err < 0)
    {
        log_printf("load phdr failed\n");
        goto load_failed;
    }
    sys_close(file);
    task->heap_start = elf_phdr.p_vaddr + elf_phdr.p_memsz;
    task->heap_end = task->heap_start;
    return elf_hdr.e_entry;
load_failed:
    if (file)
    {
        sys_close(file);
    }
    return 0;
}

static int copy_args(char *to, uint32_t page_dir, int argc, char **argv)
{
    task_args_t task_args;
    task_args.argc = argc;
    task_args.argv = (char **)(to + sizeof(task_args_t));

    char *dest_arg = to + sizeof(task_args_t) + sizeof(char *) * argc;
    char **dest_arg_tb = (char **)memory_get_paddr(page_dir, (uint32_t)(to + sizeof(task_args_t)));
    for (int i = 0; i < argc; i++)
    {
        char *from = argv[i];
        int len = kernel_strlen(from) + 1;
        int err = memor_copy_uvm_data((uint32_t)dest_arg, page_dir, (uint32_t)from, len);
        ASSERT(err >= 0);
        dest_arg_tb[i] = dest_arg;
        dest_arg += len;
    }
    return memor_copy_uvm_data((uint32_t)to, page_dir, (uint32_t)&task_args, sizeof(task_args));
}

int sys_execve(char *name, char **argv, char **env)
{
    task_t *task = task_current();

    kernel_strncpy(task->name, get_file_name(name), TASK_NAME_SIZE);

    uint32_t old_page_dir = task->tss.cr3;
    uint32_t new_page_dir = memory_create_uvm();

    if (!new_page_dir)
    {
        goto execve_failed;
    }

    uint32_t entry = load_elf_file(task, name, new_page_dir);

    if (entry == 0)
    {
        goto execve_failed;
    }
    uint32_t stack_top = MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE;
    int err = memory_alloc_for_page_dir(
        new_page_dir,
        MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE,
        MEM_TASK_STACK_SIZE,
        PTE_P | PTE_U | PTE_W);
    if (err < 0)
    {
        goto execve_failed;
    }

    int arg_num = string_count(argv);
    err = copy_args((char *)stack_top, new_page_dir, arg_num, argv);
    if (err < 0)
    {
        goto execve_failed;
    }
    syscall_frame_t *frame = (syscall_frame_t *)(task->tss.esp0 - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = frame->edx = frame->ecx = frame->ebx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFALGS_IF | EFALGS_DEFAULT;
    frame->esp = stack_top - sizeof(uint32_t) * SYSCALL_PARAM_COUNT;

    // 更新当前的页目录
    task->tss.cr3 = new_page_dir;
    mmu_set_page_dir(new_page_dir);

    memory_destroy_uvm(old_page_dir);
    return 0;
execve_failed:
    if (new_page_dir)
    {
        task->tss.cr3 = old_page_dir;
        mmu_set_page_dir(old_page_dir);
        memory_destroy_uvm(new_page_dir);
    }
    return -1;
}

void sys_exit(int status)
{
    task_t *curr_task = task_current();
    for (int fd = 0; fd < TASK_OFILE_NR; fd++)
    {
        file_t *file = curr_task->file_table[fd];
        if (file)
        {
            sys_close(fd);
            curr_task->file_table[fd] = (file_t *)0;
        }
    }

    int move_child = 0;

    mutex_lock(&task_table_mutex);
    for (int i = 0; i < TASK_NR; i++)
    {
        task_t *task = task_table + i;
        if (task->parent == curr_task)
        {
            task->parent = (task_t *)0;
            move_child = 1;
        }
    }
    mutex_unlock(&task_table_mutex);

    int state = irq_enter_protection();
    // 如果有移动子进程，则唤醒init进程
    task_t *parent = curr_task->parent;
    if (move_child && (parent != &task_manager.first_task))
    {
        if (task_manager.first_task.state == TASK_WAITTING)
        {
            task_set_ready(&task_manager.first_task);
        }
    }

    // 如果有父任务在wait，则唤醒父任务进行回收
    // 如果父进程没有等待，则一直处理僵死状态？
    if (parent->state == TASK_WAITTING)
    {
        task_set_ready(parent);
    }

    curr_task->exit_status = status;
    curr_task->state = TASK_ZOMBIE;
    task_set_block(curr_task);
    task_dispatch();
    irq_leave_protection(state);
}

int sys_wait(int *status)
{
    task_t *curr_task = task_current();
    for (;;)
    {
        mutex_lock(&task_table_mutex);
        for (int i = 0; i < TASK_NR; i++)
        {
            task_t *task = task_table + i;
            if (task->parent != curr_task)
            {
                continue;
            }

            if (task->state == TASK_ZOMBIE)
            {
                int pid = task->pid;
                *status = task->exit_status;
                memory_destroy_uvm(task->tss.cr3);
                memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
                kernel_memset((void *)task, 0, sizeof(task_t));
                mutex_unlock(&task_table_mutex);
                return pid;
            }
        }
        mutex_unlock(&task_table_mutex);
        irq_state_t state = irq_enter_protection();
        task_set_block(curr_task);
        curr_task->state = TASK_WAITTING;
        task_dispatch();
        mutex_unlock(&task_table_mutex);
    }

    return 0;
}