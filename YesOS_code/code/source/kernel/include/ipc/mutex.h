#ifndef MUTEX_H
#define MUTEX_H

#include "tools/list.h"
#include "core/task.h"

typedef struct _mutex_t
{
    task_t * owner;
    int locked_count;
    list_t wait_list;
} mutex_t;

/**
 * 锁初始化
 */
void mutex_init(mutex_t *mutex);

/**
 * 上锁
 */
void mutex_lock(mutex_t *mutex);

/**
 * 解锁
 */
void mutex_unlock(mutex_t *mutex);

#endif