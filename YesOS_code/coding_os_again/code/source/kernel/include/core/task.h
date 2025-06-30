/**
 * @FilePath     : /code/source/kernel/include/core/task.h
 * @Description  :  (多任务)task相关头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-29 16:41:39
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef TASK_H
#define TASK_H

#include "comm/types.h"
#include "cpu/cpu.h"
#include "tools/list.h"

#define TASK_NAME_SIZE 64          // 进程名字
#define TASK_TIME_SLICE_DEFAULT 10 // 定时中断
#define TASK_FLAGS_SYSTEM (1 << 0) // 系统进程
/**
 * 进程控制块结构
 */
typedef struct _task_t
{
    enum
    {
        TASK_CREATED, // 创建
        TASK_RUNNING, // 运行
        TASK_SLEEP,   // 延时
        TASK_READY,   // 就绪
        TASK_WAIT,    // 等待
    } state;

    int time_ticks;            // 进程运行时间片
    int slice_ticks;           // 进程已运行时间
    int sleep_ticks;           // 进程睡眠时间
    char name[TASK_NAME_SIZE]; // 任务名

    list_node_t run_node;  // 运行相关结点
    list_node_t all_node;  // 所有队列结点
    list_node_t wait_node; // 等待结点

    // uint32_t * stack;
    tss_t tss;        // 任务的tss段
    uint32_t tss_sel; // tss选择子
} task_t;

/**
 * @brief        : 进程初始化
 * @param         {task_t *} task: 需要运行的进程的指针
 * @param         {const char *} name : 进程名称
 * @param         {int} flag: 1为系统进程,0为普通进程
 * @param         {uint32_t} entry: 入口地址(解析elf的得到的入口地址)
 * @param         {uint32_t} esp: 栈顶指针
 * @return        {int} : 状态码0
 **/
int task_init(task_t *task, const char *name, int flag, uint32_t entry, uint32_t esp);

/**
 * @brief        : 实现从一个任务到另一个任务的切换
 * @param         {task_t *} from: 当前任务
 * @param         {task_t *} to: 切换到的任务
 * @return        {*}
 **/
void task_switch_from_to(task_t *from, task_t *to);

/**
 * 任务管理器
 */
typedef struct _task_manager_t
{
    list_t ready_list; // 就绪队列
    list_t task_list;  // 所有已经创建的任务
    task_t first_task; // 第一个任务
    task_t *curr_task; // 当前运行的任务
    list_t sleep_list; // 进程睡眠队列

    task_t idle_task; // 空闲进程

    int app_code_sel; // 代码段选择子
    int app_data_sel; // 数据段选择子
} task_manager_t;

// 任务管理器初始化
void task_manager_init(void);

// 初始化第一个进程(任务)
void task_first_init(void);

// 返回第一个进程(任务)
task_t *task_first_task(void);

void task_set_block(task_t *task);
int sys_sched_yield(void); // 进程主动放弃cpu
void task_dispatch(void);
task_t *task_current(void);
void task_time_ticks(void);
void sys_sleep(uint32_t ms); // 进程延时
void task_set_sleep(task_t *task, uint32_t ticks);
void task_set_wakeup(task_t *task);
void task_set_ready(task_t *task);
#endif