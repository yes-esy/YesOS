/**
 * @FilePath     : /code/source/kernel/fs/file.c
 * @Description  :  文件相关实现.c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 13:20:23
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "fs/file.h"
#include "ipc/mutex.h"
#include "tools/klib.h"
static file_t file_table[FILE_TABLE_SIZE];
static mutex_t file_alloc_mutex; //  文件分配互斥锁
/**
 * @brief        : 从文件表中分配一个空闲表项
 * @return        {file_t*} : 分配的表项的指针
 **/
file_t *file_alloc(void)
{
    file_t *file = (file_t *)0;    // 记录找到的空闲表项的指针
    mutex_lock(&file_alloc_mutex); // 上锁
    // 遍历表项
    for (int i = 0; i < FILE_TABLE_SIZE; i++)
    {
        file_t *cur_file = file_table + i; // 当前表项
        if (cur_file->ref == 0)            // 当前文件没有被打开过
        {
            kernel_memset((void *)cur_file, 0, sizeof(cur_file)); // 清空该表项;
            cur_file->ref = 1;
            file = cur_file;
            break;
        }
    }
    mutex_unlock(&file_alloc_mutex); // 解锁
    return file;
}
/**
 * @brief        : 从文件表释放文件
 * @param         {file_t *} file: 要释放的文件
 * @return        {void}
 **/
void file_free(file_t *file)
{
    mutex_lock(&file_alloc_mutex); // 上锁
    if (file->ref)                 // 文件打开次数减1
    {
        file->ref--;
    }
    mutex_unlock(&file_alloc_mutex); // 解锁
}
/**
 * @brief        : 增加文件的使用次数
 * @param         {file_t *} file: 需要增加的文件
 * @return        {void}
 **/
void file_incr_ref(file_t *file)
{
    mutex_lock(&file_alloc_mutex);   // 上锁
    file->ref++;                     // 打开次数增加
    mutex_unlock(&file_alloc_mutex); // 解锁
}
/**
 * @brief        : 对文件表初始化
 * @return        {void}
 **/
void file_table_init(void)
{
    mutex_init(&file_alloc_mutex);                            // 初始化互斥锁
    kernel_memset((void *)file_table, 0, sizeof(file_table)); // 清空文件表
}