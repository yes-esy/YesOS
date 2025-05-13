/**
 * @FilePath     : /code/source/kernel/ipc/sem.c
 * @Description  :  sem信号量.c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-10 17:13:19
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "ipc/sem.h"
#include "core/task.h"
#include "cpu/irq.h"
/**
 * @brief        : 信号量初始化，初始化等待队列，计数值等
 * @param         {sem_t} *sem: 待初始化的信号量
 * @param         {int} init_count: 初始计数值
 * @return        {*}
 **/
void sem_init(sem_t *sem, int init_count)
{
    sem->count = init_count;
    list_init(&sem->wait_list);
}

/**
 * @brief        : 等信号（p，wait)，如果当前信号量的计数值为0，则当前进程需等待，调度下一进程执行；反之。计数值减一，并继续执行
 * @param         {sem_t} *sem: 信号量
 * @return        {*}
 **/
void sem_wait(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();
    if (sem->count > 0)
    {
        sem->count--;
    }
    else
    {
        task_t *curr = task_current();
        task_set_block(curr);                                // 阻塞当前进程
        list_insert_last(&sem->wait_list, &curr->wait_node); // 插入到等待队列中
        task_dispatch();                                     // 调度下一进程
    }
    irq_leave_protection(state);
}

/**
 * @brief        : 发信号（v,signal），某些事件完成或某些资源到达，等待队列中有进程则插入就绪队列，否则，信号量计数值加1，
 * @return        {*}
 **/
void sem_signal(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();
    if (list_count(&sem->wait_list)) // 等待队列中是否有进程在等待
    {
        list_node_t *node = list_remove_first(&sem->wait_list);   // 有则将取出等待队列中第一个进程
        task_t *task = list_node_parent(node, task_t, wait_node); // 找到该进程
        task_set_ready(task);                                     // 插入就绪队列
        task_dispatch();                                          // 调度下一个进程
    }
    else // 等待队列没有进程等待
    {
        sem->count++; // 计数值++，表示有新的资源
    }
    irq_leave_protection(state);
}
/**
 * @brief        : 返回当前信号量的计数值
 * @param         {sem_t} *sem: 须知晓计数值的信号量
 * @return        {int} 当前信号量的计数值
 **/
int sem_count(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();
    int count = sem->count;
    irq_leave_protection(state);
    return count;
}
