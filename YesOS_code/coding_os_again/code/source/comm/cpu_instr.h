/**
 * @FilePath     : /code/source/comm/cpu_instr.h
 * @Description  :  汇编指令的封装
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-30 20:56:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef CPU_INSTR_H
#define CPU_INSTR_H

#include "types.h"

/**
 * @brief        : 关中断
 * @return        {*}
 **/
static inline void cli(void)
{
    __asm__ __volatile__("cli");
}

/**
 * @brief        : 开中断
 * @return        {*}
 **/
static inline void sti(void)
{
    __asm__ __volatile__("sti");
}

/**
 * @brief        :  从上端口读取一个字
 * @param         {uint16_t} port:端口号
 * @return        {uint16_t} 读取的字
 **/
static inline uint16_t inw(uint16_t port)
{
    uint16_t rv;
    /**
     * inb al,dx
     */
    __asm__ __volatile__("in %1, %0" : "=a"(rv) : "dN"(port));
    return rv;
}

/**
 * @brief        :从I/O端口读取一个字节
 * @param         {uint16_t} port:端口号
 * @return        {uint8_t}读取的字节
 **/
static inline uint8_t inb(uint16_t port)
{
    uint8_t rv;
    /**
     * inb al,dx
     */
    __asm__ __volatile__("inb %[p],%[v]" : [v] "=a"(rv) : [p] "d"(port));
    return rv;
}

/**
 * @brief        : 往指定端口写数据
 * @param         {uint16_t} port:端口号
 * @param         {uint8_t} data:要写的数据
 * @return        {*}
 **/
static inline void outb(uint16_t port, uint8_t data)
{
    /**
     * outb al,dx
     */
    __asm__ __volatile__("outb %[v],%[p]" ::[p] "d"(port), [v] "a"(data));
}

/**
 * @brief        : 加载gdt表(全局描述符)
 * @param         {uint32_t} start: 起始地址
 * @param         {uint32_t} size: 大小
 * @return        {*}
 **/
static inline void lgdt(uint32_t start, uint32_t size)
{
    // gdt表的信息
    struct
    {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } gdt;
    gdt.start31_16 = start >> 16;   // 获取高16位
    gdt.start15_0 = start & 0xFFFF; // 获取低16位
    gdt.limit = size - 1;           // 大小
    __asm__ __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}

/**
 * @brief        :  加载idt的地址到idtr寄存器
 * @param         {uint32_t} start: 起始地址
 * @param         {uint32_t} size: 大小
 * @return        {*}
 **/
static inline void lidt(uint32_t start, uint32_t size)
{
    // gdt表的信息
    struct
    {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } idt;
    idt.start31_16 = start >> 16;   // 获取高16位
    idt.start15_0 = start & 0xFFFF; // 获取低16位
    idt.limit = size - 1;           // 大小
    __asm__ __volatile__("lidt %[g]" ::[g] "m"(idt));
}

/**
 * @brief        : 写入cr3
 * @param         {uint32_t} v: 写入的值
 * @return        {*}
 **/
static inline void write_cr3(uint32_t v)
{
    __asm__ __volatile__("mov %[v],%%cr3" ::[v] "r"(v));
}

/**
 * @brief        : 读取cr3
 * @return        {uint32_t} cr3的值
 **/
static inline uint32_t read_cr3(void)
{
    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3,%[v]" : [v] "=r"(cr3));
    return cr3;
}

/**
 * @brief        : 写入cr4
 * @param         {uint32_t} v: 写入的值
 * @return        {*}
 **/
static inline void write_cr4(uint32_t v)
{
    __asm__ __volatile__("mov %[v],%%cr4" ::[v] "r"(v));
}

/**
 * @brief        : 读取cr4
 * @return        {uint16_t} cr4的值
 **/
static inline uint32_t read_cr4(void)
{
    uint32_t cr4;
    __asm__ __volatile__("mov %%cr4,%[v]" : [v] "=r"(cr4));
    return cr4;
}
/**
 * @brief        : 读取cr0
 * @return        {uint32_t} cr0的值
 **/
static inline uint32_t read_cr0(void)
{
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0,%[v]" : [v] "=r"(cr0));
    return cr0;
}
/**
 * @brief        : 写入cr0
 * @param         {uint32_t} v: 写入的值
 * @return        {void} : 无返回值
 **/
static inline void write_cr0(uint32_t v)
{
    __asm__ __volatile__("mov %[v],%%cr0" ::[v] "r"(v));
}
/**
 * @brief        : 远跳转指令
 * @param         {uint32_t} selector: 选择子
 * @param         {uint32_t} offset: 偏移量
 * @return        {*}
 **/
static inline void far_jump(uint32_t selector, uint32_t offset)
{
    uint32_t addr[] = {offset, selector};
    __asm__ __volatile__("ljmpl *(%[a])" ::[a] "r"(addr));
}
/**
 * @brief        : 停机指令
 * @return        {*}
 **/
static inline void hlt(void)
{
    __asm__ __volatile__("hlt");
}

/**
 * @brief        : 写task register寄存器于lgdt类似
 * @param         {uint16_t} tss_selector: 对应的tss选择子
 * @return        {*}
 **/
static inline void write_tr(uint16_t tss_selector)
{
    __asm__ __volatile__("ltr %%ax" ::"a"(tss_selector));
}
/**
 * @brief        : 读出eflags寄存器的内容
 * @return        {uint32_t}
 **/
static inline uint32_t read_eflags(void)
{
    uint32_t eflags;
    __asm__ __volatile__("pushf \n\tpop %%eax" : "=a"(eflags));
    return eflags;
}
/**
 * @brief        : 往eflags寄存器写入内容
 * @param         {uint32_t} eflags: 写入的值
 * @return        {void} 无返回值
 **/
static inline void write_eflags(uint32_t eflags)
{
    __asm__ __volatile__("push %%eax\n\tpopf" ::"a"(eflags));
}
/**
 * @brief        : 读取cr2
 * @return        {uint32_t}: 读取出的cr2的值
 **/
static inline uint32_t read_cr2(void)
{
    uint32_t cr2;
    __asm__ __volatile__("mov %%cr0,%[v]" : [v] "=r"(cr2));
    return cr2;
}
/**
 * @brief        : 写入cr2
 * @param         {uint32_t} v: 写入cr2的值
 * @return        {void} : 无返回值
 **/
static inline void write_cr2(uint32_t v)
{
    __asm__ __volatile__("mov %[v],%%cr0" ::[v] "r"(v));
}
#endif