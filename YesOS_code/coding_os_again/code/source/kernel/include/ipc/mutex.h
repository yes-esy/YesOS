/**
 * @FilePath     : /code/source/kernel/include/ipc/mutex.h
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-09 21:16:03
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef MUTEX_H
#define MUTEX_H

#include "tools/list.h"
#include "core/task.h"

typedef struct _mutex_t
{
    list_t wait_list; // 等待队列
    task_t *owner;    // 锁的拥有进程
    int locked_count; // 上锁次数
} mutex_t;

// 初始化
void mutex_init(mutex_t *mutex);

// 上锁
void mutex_lock(mutex_t *mutex);

// 解锁
void mutex_unlock(mutex_t *mutex);
#endif