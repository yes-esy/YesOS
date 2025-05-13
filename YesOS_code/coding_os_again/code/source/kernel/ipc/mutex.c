/**
 * @FilePath     : /code/source/kernel/ipc/mutex.c
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-10 19:48:44
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "ipc/mutex.h"
#include "cpu/irq.h"
#include "core/task.h"

/**
 * @brief        : 互斥锁的初始化，包括上锁次数，锁的拥有进程为空，初始化等待队列
 * @param         {mutex_t} *mutex: 待初始化的互斥锁
 * @return        {*}
 **/
void mutex_init(mutex_t *mutex)
{
    mutex->locked_count = 0;
    mutex->owner = (task_t *)0;
    list_init(&mutex->wait_list);
}

/**
 * @brief        : 对互斥锁进行上锁，未上锁则上锁（次数加1，拥有者为上锁进程）；已上锁，且为当前进程上的锁，次数增加即可；已上锁，但非当前进程所上的，
 * @param         {mutex_t} *mutex: 需要上锁的互斥锁
 * @return        {*}
 **/
void mutex_lock(mutex_t *mutex)
{
    irq_state_t state = irq_enter_protection();
    task_t *curr = task_current();
    if (mutex->locked_count == 0) // 未上锁
    {
        mutex->locked_count++; // 上锁
        mutex->owner = curr;   // 锁的拥有者为当前执行的进程
    }
    else if (mutex->owner == curr) // 已经上锁，且为当前进程再次上锁
    {
        mutex->locked_count++; // 上锁次数增加即可
    }
    else // 已经上锁，且为非当前进程再次上锁
    {
        task_set_block(curr);                                  // 当前进程需要等待，等待锁释放
        list_insert_last(&mutex->wait_list, &curr->wait_node); // 插入到等待队列
        task_dispatch();                                       // 调度下一个任务
    }
    irq_leave_protection(state);
}

/**
 * @brief        : 对互斥锁进行解锁操作
 * @details      : 释放当前进程持有的互斥锁。如果是可重入锁，则减少锁计数；
 *                 当计数为0时完全释放锁并唤醒等待队列中的第一个进程。
 *                 只有锁的持有者才能成功解锁。
 * @param         {mutex_t} *mutex: 待解锁的互斥锁
 * @return        {void}
 * @note         : 此函数在中断保护状态下执行，确保操作的原子性
 **/
void mutex_unlock(mutex_t *mutex)
{
    irq_state_t state = irq_enter_protection();

    task_t *curr = task_current(); // 获取当前进程

    if (mutex->owner == curr) // 只有锁的持有者才能解锁
    {
        if (--mutex->locked_count == 0) // 锁完全释放
        {
            mutex->owner = (task_t *)0; // 锁的拥有者清空

            if (list_count(&mutex->wait_list)) // 等待队列非空
            {
                list_node_t *task_node = list_remove_first(&mutex->wait_list); // 取出等待队列头节点（第一个进程）

                task_t *task = list_node_parent(task_node, task_t, wait_node); // // 从节点获取对应的进程

                task_set_ready(task); // // 将任务从阻塞状态改为就绪状态

                mutex->locked_count = 1; // 当前进程获得该所，即上锁一次

                mutex->owner = task; // 更新锁的拥有者为当前进程

                task_dispatch(); // 调度该进程
            }
        }
    }

    irq_leave_protection(state);
}