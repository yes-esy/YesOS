/**
 * @FilePath     : /code/source/kernel/include/cpu/mmu.h
 * @Description  :  虚拟内存管理单元头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-26 16:51:36
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef MMU_H
#define MMU_H

#define PDE_CNT (1024) // 页目录表项数量
#define PTE_P (1 << 0) // 页表表项存在
#define PDE_P (1 << 0) // 页目录表项存在
#define PDE_W (1 << 1) // 页目录表项可写
#define PTE_W (1 << 1) // 页表表项可写
#define PDE_U (1 << 2) // 页目录表项用户态访问

#include "comm/types.h"
#include "comm/cpu_instr.h"
#pragma pack(1)
/**
 * Page-Directory Entry 页目录项描述结构
 */
typedef union _pde_t
{
    uint32_t v;
    struct
    {
        uint32_t present : 1;       // 是否存在
        uint32_t write_enable : 1;  // 是否可写
        uint32_t user_mode_acc : 1; // 用户模式是否可以访问
        uint32_t write_through : 1; // 决定是否使用写直达缓存策略
        uint32_t cache_disable : 1; // 禁止对该页的缓存
        uint32_t accessed : 1;      // 是否访问过
        uint32_t : 1;               // 保留位，目前没有特定用途，匿名
        uint32_t ps : 1;            // 用于指示大页支持。如果为1，表示这是一个大页条目；如果为0，表示这是一个标准的页表条目。
        uint32_t : 4;               // 未使用到,匿名
        uint32_t phy_pt_addr : 20;  // 二级页表对应的物理地址
    };
} pde_t;
/**
 * Page-Table Entry 页表表项描述结构
 */
typedef union _pte_t
{
    uint32_t v;
    struct
    {
        uint32_t present : 1;        // 是否存在
        uint32_t write_enable : 1;   // 是否可写
        uint32_t user_mode_acc : 1;  // 用户模式是否可以访问
        uint32_t write_through : 1;  // 决定是否使用写直达缓存策略
        uint32_t cache_disable : 1;  // 如果为1，禁止对该页的缓存；如果为0，允许缓存
        uint32_t accessed : 1;       // 记录该页是否被访问过
        uint32_t dirty : 1;          // 表示该页的内容是否已被修改
        uint32_t pat : 1;            // 用于间接确定访问该页所使用的内存类型，
        uint32_t global : 1;         // 该位决定翻译是否是全局的。全局页不会被TLB刷新指令清除，从而提高性能
        uint32_t : 3;                // 未使用到,匿名
        uint32_t phy_page_addr : 20; // 页的物理地址，用于指向实际的物理内存位置
    };
} pte_t;
#pragma pack()
/**
 * @brief        : 将页目录表的首地址写入cr3寄存器
 * @param         {uint32_t} paddr:页目录表地址
 * @return        {*}
 **/
static inline void mmu_set_page_dir(uint32_t paddr)
{
    write_cr3(paddr);
}
/**
 * @brief        : 取出页目录项在页目录表中的索引
 * @param         {uint32_t} vaddr: 对应的虚拟地址
 * @return        {uint_32} : 返回页目录项在页目录中的索引
 **/
static inline uint32_t pde_index(uint32_t vaddr)
{
    int index = (vaddr >> 22);
    return index;
}
/**
 * @brief        : 取出物理页在页表中的索引
 * @param         {uint32_t} vaddr: 对应的虚拟地址
 * @return        {uint32_t} : 返回物理页在页表中的索引
 **/
static inline uint32_t pte_index(uint32_t vaddr)
{
    return (vaddr >> 12) & 0x3FF; // 取中间10位
}
/**
 * @brief        : 取出页目录表项对应的页表物理地址,pde->phy_pt_addr为后20位
 * @param         {pde_t} *pde: 页目录表项
 * @return        {uint32_t} : 页表的物理地址
 **/
static inline uint32_t pde_paddr(pde_t *pde)
{
    return pde->phy_pt_addr << 12;
}
/**
 * @brief        : 取出页表项对应的页的物理地址,pte->phy_page_addr为后20为
 * @param         {pte_t} *pte: 页表的物理地址
 * @return        {uint32_t} : 页的物理地址
 **/
static inline uint32_t pte_paddr(pte_t *pte)
{
    return pte->phy_page_addr << 12;
}

#endif