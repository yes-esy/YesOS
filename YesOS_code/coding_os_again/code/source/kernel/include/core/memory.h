/**
 * @FilePath     : /code/source/kernel/include/core/memory.h
 * @Description  :  内存管理头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-29 19:03:23
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef MEMORY_H
#define MEMORY_H

#define MEM_EXT_START (1024 * 1024) // 1MB
#define MEM_PAGE_SIZE 4096          // 页大小
#define MEM_EBDA_START 0x80000
#define MEMORY_TASK_BASE (0x80000000) // 进程起始地址空间
#define MEM_EXT_END (127 * 1024 * 1024)
#include "comm/types.h"
#include "tools/bitmap.h"
#include "ipc/mutex.h"
#include "comm/boot_info.h"
/**
 * @brief 地址分配结构
 */
typedef struct _addr_alloc_t
{
    bitmap_t bitmap;    // 位图
    uint32_t start;     // 起始地址
    uint32_t size;      // 大小
    uint32_t page_size; // 页大小,块大小
    mutex_t mutex;      // 互斥信号量
} addr_alloc_t;

typedef struct _memory_map_t
{
    void *vstart;  // 线性地址起始地址（虚拟地址）
    void *vend;    // 结束地址
    void *pstart;  // 物理地址
    uint32_t perm; // 特权相关属性
} memory_map_t;

// 初始化，对整个OS内存管理初始化

/**
 * @brief        : 内存初始化
 * @param         {boot_info_t} *boot_info: 启动信息
 * @return        {*}
 **/
void memory_init(boot_info_t *boot_info);
/**
 * @brief        : 创建页目录表,并将内核页目录表中的表项拷贝到新创建的页目录表中
 * @return        {uint32_t}: 页目录表物理地址
 **/
uint32_t memory_create_uvm();
/**
 * @brief        : 为线性地址区域分配页表内存空间
 * @param         {uint32_t} addr: 需要分配内存区域的程序的线性起始地址
 * @param         {uint32_t} size: 内存空间大小
 * @param         {int} perm: 权限
 * @return        {int} : 状态码
**/
int memory_alloc_page_for(uint32_t addr , uint32_t size ,int perm);

/**
 * @brief        : 分配一页物理内存
 * @return        {uint32_t}
**/
uint32_t memory_alloc_page(void);
/**
 * @brief        : 释放一页物理内存
 * @param         {uint32_t} addr:
 * @return        {*}
**/
void memory_free_page(uint32_t addr);
#endif